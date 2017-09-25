/* Christopher Bachelor
** cbachelor@ucla.edu
** UCLA ID: 004608570 */

#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <mcrypt.h>

#define BUFFER_SIZE 1024   /*Max buffer we read in */
int socketfd;               /*File descriptors we use in the program*/
int logfd;
int logOn;                  /*Statuses of passed command options. 1 if passed, 0 if not*/
int keyOn;
int keySize;
char* IV = "AAAAAAAAAAAAAAAA";  /*The IV used for encrypt & decrypt*/
struct termios saved_attr;      /*Save current terminal settings*/

/*Function called when exiting program. Restores terminal modes*/
void exitClient(int err) {
    
    if(close(socketfd) < 0){
    	fprintf(stderr, "%s", strerror(errno));
    }
    if(tcsetattr(0, TCSANOW, &saved_attr) == -1){
        fprintf(stderr, "Error resetting attributes\r\n"); 
    }
    exit(err);
}

/*Error function used to print error messages*/
void printError(int ERROR_NO) {
    fprintf(stderr, "%s\n", strerror(ERROR_NO));
    exitClient(1);
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

/*Function changes the terminal settings to no echo one byte at a time mode. Saves old terminal
attributes.*/
void changeTerminal() {
    int ret_saved, ret_copy;
    struct termios copy_attr;
    /*Check if STDIN is a terminal */
    if (!isatty(STDIN_FILENO)) { 
        printError(errno); 
    }
    /*Save current terminal attributes, and a copy that we change*/
    ret_saved = tcgetattr(STDIN_FILENO, &saved_attr);
    ret_copy = tcgetattr(STDIN_FILENO, &copy_attr);
    if (ret_saved == -1 || ret_copy == -1) { 
        printError(errno); 
    }
    /*Change values of copied struct*/
    copy_attr.c_iflag &= 0;
    copy_attr.c_iflag |= ISTRIP;
    copy_attr.c_oflag &= 0;
    copy_attr.c_lflag &= 0;
    ret_copy = tcsetattr(STDIN_FILENO, TCSANOW, &copy_attr);
    if (ret_copy == -1) { 
        printError(errno); 
    }
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
    ret = mdecrypt_generic(decrypt_td, buffer, buffer_len);
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
        fprintf(stderr, "Failed to encrypt message\r\n");
        exit(1);
    }
     ret = mcrypt_generic_deinit(encrypt_td);
    if (ret < 0) {
        fprintf(stderr, "Failed to deinitialize encryptor\r\n");
        exit(1);
    }
    mcrypt_module_close(encrypt_td);
}

/*Function interprets inputs from keyboard, echoes it out, and sends it to server.
  If --encrypt is on, message sent to server is encrypted beforehand.
  If --log is on, it records the sent message to a file */
void readInput(char* buf, ssize_t bytesRead, char* keyFile) {
    char serverMessage[bytesRead];
    int i, k;
    int crlfCount = 0;
    ssize_t bytesWrite;
    for(k = 0; k < bytesRead; k++) {
    	if (buf[k] == 0x0D || buf[k] == 0x0A) {
	    crlfCount++;
    	}
    }
    char echoMessage[bytesRead + crlfCount];
    k = 0;
    /*Interpret characters one byte at a time, write out as a chunk*/
    for(i = 0; i < bytesRead; i++) 
    {
    	switch (buf[i]) {
        	case 0x0D:
        	case 0x0A:
        	    /*Handle CR and LF cases. In both cases echo CRLF, and write only LF to server*/
        	    serverMessage[i] = '\n';
        	    echoMessage[k] = '\r';
        	    k++;
        	    echoMessage[k] = '\n';
        	    break;
        	default:
        	    serverMessage[i] = buf[i];
        	    echoMessage[k] = buf[i];
        	}
    	k++;
    }
    bytesWrite = write(STDOUT_FILENO, echoMessage, bytesRead + crlfCount);
    if (bytesWrite < 0) {
    	printError(errno);
    }
    if(keyOn) {
        /*Encrypt if flag is on*/
        encrypt(serverMessage, bytesRead, keyFile);
    }
    bytesWrite = write(socketfd, serverMessage, bytesRead);
    if (bytesWrite < 0) {
    	printError(errno);
    }
    /*Write to log*/
    if(logOn) {
        dprintf(logfd, "SENT %d bytes: ", bytesRead);
        bytesWrite = write(logfd, serverMessage, bytesRead);
        if (bytesWrite < 0) {
            printError(errno);
        }
        dprintf(logfd, "\n");
    }
}

/*Function interprets inputs from server, echoes it out.
  If --encrypt is on, message echoed is decrypted beforehand.
  If --log is on, it records the received message to a file */
void readServer(char* buf, int bytesRead, char* keyFile) {
    ssize_t bytesWrite;
    char crlf[2] = {'\r', '\n'};
    char serverMessage[bytesRead];
    
    char ch;
    int crlfCount = 0;

    int i, k;
    for(k = 0; k < bytesRead; k++) {
        if ( buf[k] == 0x0A) {
            crlfCount++;
        }
    }
    k = 0;
    char echoMessage[bytesRead + crlfCount];
    for (i = 0; i < bytesRead; i++) {
    	ch = buf[i];        
        echoMessage[i] = buf[i];
        serverMessage[i] = buf[i]; 
    }   

    if(keyOn ==1) {
        decrypt(echoMessage, bytesRead, keyFile);
    }
    /*Writes out to echo*/
    for( i =0; i < bytesRead; i++) {
        if(echoMessage[i] == 0x0A) {
             bytesWrite = write(STDOUT_FILENO, crlf, 2);
            if (bytesWrite < 0) {
                printError(errno);
            } 
        }
           else {
             bytesWrite = write(STDOUT_FILENO, echoMessage+i, 1);
            if (bytesWrite < 0) {
                printError(errno);
            } 
        }
    }
    /*Write out to logfile*/
    if(logOn) {
	   dprintf(logfd, "RECEIVED %d bytes: ", bytesRead);
	   bytesWrite = write(logfd, serverMessage, bytesRead);
    	if (bytesWrite < 0) {
    	    printError(errno);
    	} 
	   dprintf(logfd, "\n");
    }
}

/*Sets up poll between keyboard and server input, and calls the read that corresponds to it*/
void poll_setup(char* keyFile) {
    char buf[BUFFER_SIZE];
    ssize_t bytesRead;
    struct pollfd fds[2] =  
	{
	    {STDIN_FILENO, POLLIN, 0},
	    {socketfd, POLLIN, 0}
	};
    while(1) {
    	if(poll(fds, 2, 0) == -1) {
            printError(errno);
        }
	   if(fds[0].revents & POLLIN) {
	    /*Read in input from stdin.
	      Read in a large buffer in case multiple characters 
	      are written in at once */
	       bytesRead = read(STDIN_FILENO, buf, BUFFER_SIZE);
            if (bytesRead == -1) {
                printError(errno);
            }
            readInput(buf, bytesRead, keyFile);
        }
        if (fds[1].revents & POLLIN) {
            /*Read in input from the server output*/
            bytesRead = read(socketfd, buf, BUFFER_SIZE);
            if (bytesRead == -1) {
                printError(errno);
            }
	    
	    if (bytesRead == 0) {
            /*If reading 0 bytes from server, means it closed. So exit*/
            exitClient(0);
		}
        else
            readServer(buf, bytesRead, keyFile);
	}
	/*Handles cases when poll reports that the shell/terminal processes closed */
        if (fds[0].revents & (POLLERR+POLLHUP)) {
            exitClient(0);
        }
        if (fds[1].revents & (POLLERR+POLLHUP)) {
            exitClient(0);
        }
    }
}

int main(int argc, char* argv[]) {
    int port, ret;
    logOn = 0;
    keyOn = 0;
    char* logFile = NULL;
    char* portStr = NULL;
    char keyFile[16];
    char* keyTemp = NULL;

    struct hostent *server_name;
    struct sockaddr_in server_addr;

    /*Properly interpret command line arguments*/
    static struct option command_arg[] =
	{
	    {"port", required_argument, 0, 'p'},
	    {"log", required_argument, 0, 'l'},
	    {"encrypt", required_argument, 0, 'e'},
	    {0, 0, 0, 0}
	};
    ret = 0;
    int option_index = 0;
    while (ret != -1) {
        ret = getopt_long(argc, argv, "p:l:e:", command_arg, &option_index);
        switch (ret) {
        	case 'p':
        	    portStr = optarg;
        	    break;
        	case 'l':
        	    logFile = optarg;
        	    logOn = 1;
        	    break;
        	case 'e':
        	    keyTemp = optarg;
                keyOn = 1;
        	    break;
        	case -1:
        	    break;
        	default:
        	    fprintf(stderr, "Usage: lab1b-client --port=PORT_NUMBER [-l FILENAME] [-e FILENAME]\n");
        	    exit(1);
                }
            }
    if (portStr == NULL) {
    	fprintf(stderr, "Error: Please specify port number\n");
    	fprintf(stderr, "Usage: lab1b-client --port=PORT_NUMBER [-l FILENAME] [-e FILENAME]\n");
        exit(1);
    }
    else {
	   port = atoi(portStr);
    }
    if (logFile != NULL) {
        logfd = creat(logFile, S_IRWXU);
        /*Handle file creating error*/
        if (logfd < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(1);
        }
    }
    else if (logOn == 1) {
        fprintf(stderr, "Error: Please specify log file\n");
        fprintf(stderr, "Usage: lab1b-client --port=PORT_NUMBER [-l FILENAME] [-e FILENAME]\n");
        exit(1);
    }

    if(keyOn == 1 && keyTemp == NULL) {
    	fprintf(stderr, "Error: Please specify key file\n");
    	fprintf(stderr, "Usage: lab1b-client --port=PORT_NUMBER [-l FILENAME] [-e FILENAME]\n");
        exit(1);
    }
    else if (keyOn){
        keySize = readFile(keyTemp, keyFile);
    }
    /*End of interpreting command line args*/

    /*Setup sockets for communication with server*/
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd < 0) {
        fprintf(stderr, "Error creating socket: %s\n",strerror(errno));	
        exit(1);
    }
    char hostName[255];
    int j = gethostname(hostName, sizeof(hostName));
    if (j < 0){
        fprintf(stderr, "Error: Host not found\n");
        exit(1);
    }
    server_name = gethostbyname("localhost");
    if(server_name == NULL) {
    	fprintf(stderr, "Error: Host not found\n");
    	exit(1);
    }

    bzero((char *) &server_addr, sizeof(server_addr));
    bcopy((char *) server_name->h_addr, (char *) &server_addr.sin_addr.s_addr,
            server_name->h_length);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); /*Function converts byte order to network byte order*/

    ret = connect(socketfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if(ret < 0) {
    	fprintf(stderr, "Error connecting to server: %s\n", strerror(errno));
    	exit(1);
    }

    /*Once communication is set up, change terminal settings, and set up poll*/
    changeTerminal();
    poll_setup(keyFile);
}
