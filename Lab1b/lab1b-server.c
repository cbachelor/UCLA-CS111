/* Christopher Bachelor
** cbachelor@ucla.edu
** UCLA ID: 004608570 */

#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>
#include <mcrypt.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024   /*Max buffer we read in */
int listenfd;               /*File descriptors we use in the program*/
int newfd;
int keyOn;                  /*Statuses of passed command options. 1 if passed, 0 if not*/
int keySize;
char* IV = "AAAAAAAAAAAAAAAA";    /*The IV used for encrypt & decrypt*/
int server2Shell[2];        /*Arrays used for Pipes*/
int shell2Server[2];
pid_t childPID;                 /*PID of child*/

void printError(int ERROR_NO);

/*Function called when exiting server. Closes child process*/
void exitServer(int err) {
	int status, check; 
    check = waitpid(childPID, &status, 0);
    if(check == -1) { printError(errno); }
    int exit_status = WEXITSTATUS(status);
	int signal_status = 0;
	if(WIFSIGNALED(status)) {
	  signal_status = WTERMSIG(status);
  	}
  	fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", 
            signal_status, exit_status);
  	close(newfd);
	exit(err);
}

/*Error function used to print error messages*/
void printError(int ERROR_NO) {
	fprintf(stderr, "%s\n", strerror(ERROR_NO));
    exitServer(1);
}

/*Function is called when --encrypt is called, and reads the contents of the file into a char buffer*/
int readFile(char* filename, char* keyFile) {
    char ch;
    int i = 0;
    int d = 1;
    int ifd = open(filename, O_RDWR);
    if (ifd < 0) {
        printError(errno);
    }
    while(d > 0) {
        d = read(ifd, &ch, 1);
        if (d < 0) {
            printError(errno);
        }
        if (i > 15) {
            break;
        }
        if (d == 0){
            break;
        } 
        keyFile[i] = ch;
        i++;
    }
    return i;
}

/*This function initializes a MCRYPT descriptor, and encrypts the given byte buffer using 
/*twofish and cfb, and deinitilizes the MCRYPT descriptor. */
void encrypt( char* buffer, int buffer_len, char* keyFile) {
    
    int ret;
    MCRYPT encrypt_td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
    if(encrypt_td == MCRYPT_FAILED) {
        fprintf(stderr, "Failed to create encrypt file descriptor\n");
        exit(1);
    }
    ret = mcrypt_generic_init(encrypt_td, keyFile, keySize, IV);
    ret = mcrypt_generic(encrypt_td, buffer, buffer_len);

    if (ret != 0) {
        fprintf(stderr, "Failed to encrypt message\n");
        exit(1);
    }

     ret = mcrypt_generic_deinit(encrypt_td);
    if (ret < 0) {
        fprintf(stderr, "Failed to deinitialize encryptor\r\n");
        exit(1);
    }
    mcrypt_module_close(encrypt_td);
}

/*This function initializes a MCRYPT descriptor, and decrypts the given byte buffer using 
/*twofish and cfb, and deinitilizes the MCRYPT descriptor. */
void decrypt( char* buffer, int buffer_len, char* keyFile) {
    int ret;
    MCRYPT decrypt_td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
    if(decrypt_td == MCRYPT_FAILED) {
        fprintf(stderr, "Failed to create decrypt file descriptor\n");
        exit(1);
    }
    mcrypt_generic_init(decrypt_td, keyFile, keySize, IV);
    /*if ( ret < 0 ) {
        fprintf(stderr, "Failed to initialize decrypt file descriptor\n");
        exit(1);
    }*/
    //fprintf(stderr, "decrypting %s\n", buffer);
     ret = mdecrypt_generic(decrypt_td, buffer, buffer_len);
    //fprintf(stderr, "decrypted form: %s\n", buffer);
    if (ret != 0) {
        fprintf(stderr, "Failed to decrypt message\r\n");
        exit(1);
    }
    ret = mcrypt_generic_deinit(decrypt_td);
    if (ret < 0) {
        fprintf(stderr, "Failed to deinitialize decryptor\r\n");
        exit(1);
    }
    mcrypt_module_close(decrypt_td);    
}

/*Function interprets inputs from client, and sends it to shell.
  If --encrypt is on, message sent to shell is decrypted beforehand*/
void readClient(char* buf, ssize_t bytesRead, char* keyFile) {
	ssize_t bytesWrite;
    char ch;
	int i, k;
	for (i = 0; i < bytesRead; i++) {
        ch = buf[i];
        if(keyOn){
            decrypt(&ch, 1, keyFile);
		}
        switch (ch) {
            /*Handle special exit signal characters*/
			case 0x04:
				k = close(server2Shell[1]);
				if (k < 0) {
					printError(errno);
				}
				return;
				break;
			case 0x03:
                /*If ^C is received, send SIGINT to shell*/
				k = kill(childPID, SIGINT);
				if (k < 0) {
					printError(errno);
				}
				break;
			default:
				bytesWrite = write(server2Shell[1], &ch, 1);
				if (bytesWrite < 0) {
					printError(errno);
				}
		}
	}
}

/*Function interprets inputs from shell, sends to client.
  If --encrypt is on, message is encrypted beforehand.*/
void readShell(char* buf, ssize_t bytesRead, char* keyFile) {
	ssize_t bytesWrite;
    if(keyOn) {
        encrypt(buf, bytesRead, keyFile);
    }
	bytesWrite = write(newfd, buf, bytesRead);
	if (bytesWrite < 0) {
		fprintf(stderr, "Error writing to client. %s\n", strerror(errno));
	}
}

/*Sets up poll between keyboard and server input, and calls the read that corresponds to it*/
void poll_setup(char * keyFile) {
	char buf[BUFFER_SIZE];
	int k;
	ssize_t bytesRead;
	struct pollfd fds[2] =  
    {
        {newfd, POLLIN, 0},
        {shell2Server[0], POLLIN, 0}
    };
    while(1) {
    	if(poll(fds, 2, 0) == -1) {
            printError(errno);
        }
        if(fds[0].revents & POLLIN) {
        	/*Read in input from cleint.
                Read in a large buffer in case multiple characters 
                are written in at once */
        	bytesRead = read(newfd, buf, BUFFER_SIZE);
            if (bytesRead == -1) {
                printError(errno);
            }
            readClient(buf, bytesRead, keyFile);
        }
        if (fds[1].revents & POLLIN) {
            /*Read in input from the shell output*/
            bytesRead = read(shell2Server[0], buf, BUFFER_SIZE);
            if (bytesRead == -1) {
                printError(errno);
            }
            readShell(buf, bytesRead, keyFile);
		}
		/*Handles cases when poll reports that the shell/terminal processes closed */
        if (fds[0].revents & (POLLERR+POLLHUP)) {
        	/*If client closes down, send a SIGTERM to shell*/
            k = kill(childPID, SIGTERM);
				if (k < 0) {
					printError(errno);
				}
            exitServer(0);
        }
        if (fds[1].revents & (POLLERR+POLLHUP)) {
        	exitServer(0);
        }
        if (bytesRead == 0) {
            /*If reading 0 bytes from client, send a SIGTERM to shell*/
            k = kill(childPID, SIGTERM);
                if (k < 0) {
                    printError(errno);
                }
            exitServer(0);
        }
    }
}

int main(int argc, char* argv[]) {
    int port_val, ret, c, d;
    char* portStr = NULL;
    keyOn = 0;
    char* keyTemp = NULL;
    char keyFile[16];
    struct sockaddr_in server_addr, client_addr;

    /*Interpret command line arguments*/
    static struct option command_arg[] =
	{
	    {"port", required_argument, 0, 'p'},
        {"encrypt", required_argument, 0, 'e'},
	    {0, 0, 0, 0}
	};
    c = 0;
    int option_index=0;
    while (c != -1) {
    	c = getopt_long(argc, argv, "p:e:", command_arg, &option_index);
    	switch (c) {
    	case 'p':
    	    portStr = optarg;
    	    break;
        case 'e':
            keyTemp = optarg;
            keyOn = 1;
    	case -1:
    	    break;
    	default:
    	    fprintf(stderr, "Usage: lab1b-server --port=PORT_NUMBER [-e FILENAME]\n");
    	    exit(1);
	   }
    }
    if(portStr == NULL) {
    	fprintf(stderr, "Error: Please specify port number\n");
    	fprintf(stderr, "Usage: lab1b-server --port=PORT_NUMBER [-e FILENAME]\n");
    	exit(1);
    }
    else {
	   port_val = atoi(portStr);
    }
    if(keyOn == 1 && keyTemp == NULL) {
        fprintf(stderr, "Error: Please specify key file\n");
        fprintf(stderr, "Usage: lab1b-client --port=PORT_NUMBER [-e FILENAME]\n");
        exit(1);
    }
    else if (keyOn){
        keySize = readFile(keyTemp, keyFile);
    }
    

    /*Open socket for communication with client*/
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) {
	   fprintf(stderr, "Error creating socket: %s\n", strerror(errno));	
	   exit(1);
    }
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_val);
    
    ret = bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if(ret < 0) {
    	fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
    	exit(1);
    } 
    listen(listenfd, 5);
    int clilen = sizeof(client_addr);
    newfd = accept(listenfd, (struct sockaddr *) &client_addr, &clilen);
    if (newfd < 0) {
    	fprintf(stderr, "Error accepting client: %d %s\n", errno, strerror(errno));
    	exit(1);
    }
    
    /*Once connection is setup, fork a bash process, set up pipes*/
    c = pipe(server2Shell);
    d = pipe(shell2Server);
    if (c < 0 || d < 0) {
    	printError(errno);
    }

    childPID = fork();
    if (childPID < 0) {
    	printError(errno);
    }
    /*Setup all proper file descriptors for interprocess communication*/
    if (childPID == 0) {
    	if(close(0) == -1)
            printError(errno); 
        if(dup(server2Shell[0]) == -1)
            printError(errno); 
        if(close(server2Shell[0]) == -1)
            printError(errno); 
        if(close(1) == -1)
            printError(errno);
        if(dup(shell2Server[1]) == -1)
            printError(errno);
        if(close(2) == -1)
            printError(errno);
        if(dup(shell2Server[1]) == -1)
            printError(errno);
        if(close(shell2Server[1]) == -1)
            printError(errno);
        if(close(shell2Server[0]) == -1)
            printError(errno);
        if(close(server2Shell[1]) == -1)
            printError(errno);
        ret = execvp("/bin/bash", NULL);
        if(ret == -1)
            printError(errno);
    }
    else {

      if(close(server2Shell[0]) == -1)
        	printError(errno);
        if(close(shell2Server[1]) == -1)
            printError(errno);
        poll_setup(keyFile);
    }


}
