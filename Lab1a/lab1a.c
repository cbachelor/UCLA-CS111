/* Christopher Bachelor
** cbachelor@ucla.edu
** UCLA ID: 004608570 */

#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/*Global constants used throughout program*/
const int BUFFER_SIZE = 1024;   /*Max buffer we read in */
struct termios saved_attr;      /*Save current terminal settings*/
int terminalToShell[2];         /*Pipes for IPC*/
int shellToTerminal[2];
int shellOn = 0;                /* 1 if --shell is passed*/
pid_t childPID;                 /*PID of child*/

void printError(int ERROR_NO);

/*When called, restore original terminal settings and exit */
void exitTerminal(int err) {
    int check;
    if(shellOn) {
        int status;   	    	
	check = waitpid(childPID, &status, 0);
        if(check == -1) { printError(errno); }
        int exit_status = WEXITSTATUS(status);
    	int signal_status = 0;
    	if(WIFSIGNALED(status)) {
    	  signal_status = WTERMSIG(status);
    	  }
	if(tcsetattr(0, TCSANOW, &saved_attr) == -1){
	  fprintf(stderr, "Error resetting attributes\r\n"); 
	}
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", 
            signal_status, exit_status);
	exit(err);
    }
    else {
      if(tcsetattr(0, TCSANOW, &saved_attr) == -1){
	fprintf(stderr, "Error resetting attributes\r\n"); 
      }
      exit(err);
    }
}
/*Function used to easily report error based on errno value */
void printError(int ERROR_NO) {
    fprintf(stderr, "%s\n", strerror(ERROR_NO));
    exitTerminal(1);
}

/*Copy current terminal settings and change it */
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
/*Signal handler that is called when SIGPIPE is received*/
void signal_handler(int signum) {
    fprintf(stderr, "Received SIGPIPE, exiting\r\n");
    exitTerminal(1);
}

/*Function that facilitates reading from keyboard, echo back and 
  write to shell if available. Error checking for writing is handled
  After the switch statement*/
int readTerminal(char* buf, ssize_t bytesRead) {
    char lf = '\n';
    char crlf[2] = {'\r', '\n'}; 
    int i, ret, k;
    ssize_t bytes2Shell, bytesWrite;
    /*Write out to stdout one character at a time, handle each case*/
    for(i = 0; i < bytesRead; i++) {
        switch (buf[i]) {
            case 0x0D:
            case 0x0A:
                /*Handle CR and LF cases. In both cases echo CRLF, and write only LF to shell*/
                bytesWrite = write(STDOUT_FILENO, crlf, 2);
                if (shellOn) {
                    bytes2Shell = write(terminalToShell[1], &lf, 1);
                    if( bytes2Shell == -1 ) {
                        printError(errno);
                    }
                }
                break;
            case 0x04:
                /*Exit properly when EOF (^D) is received. Close the pipe that writes to shell*/
                if(shellOn) {
		  ret = close(terminalToShell[1]);
		  if(ret == -1){
		      printError(errno);
		  }
		  return 1;
                } 
                else
                    exitTerminal(0);
                break;
 	    case 0x03:  /*Case when ^C is received*/
                if(shellOn) {
                    k = kill(childPID, SIGINT);
                    if(k == -1) {
                        printError(errno);
                    }
                }
                break;
            default:
                /*Echo what you read, and if possible write to shell*/
                bytesWrite = write(STDOUT_FILENO, &buf[i], 1); 
                if (shellOn) {
                    bytes2Shell = write(terminalToShell[1], &buf[i], 1);
		            if( bytes2Shell == -1 ) {
                        printError(errno);
                    }
                }
                break;
        }
        if (bytesWrite == -1) {
            printError(errno);
        }
    }   
    return 0;
}

/*Facilitates reading output from the shell only, echoes to stdout*/
void readShell(char* buf, ssize_t bytesRead) {
    char crlf[2] = {'\r', '\n'}; 
    ssize_t bytesWrite;
    int i;
    /*Write out to stdout one character at a time*/
    for(i = 0; i < bytesRead; i++) {
      //      fprintf(stderr, "char is %c\n", buf[i]);
        switch (buf[i]) {
            case 0x0A:
                /*When you receive a LF from shell, echo out CRLF*/
                bytesWrite = write(STDOUT_FILENO, crlf, 2);
                break;
            default:
                bytesWrite = write(STDOUT_FILENO, &buf[i], 1);     
                break;
        }
        if (bytesWrite == -1) {
            printError(errno);
        }
    }   
}
/* Read in from stdin and depending on if the shell exists, calls functions
   to write to echo and to shell. Uses poll() to allow simultaneous input from
   keyboard and shell */
void readWrite() {
    char buf[BUFFER_SIZE];
    char lf = '\n';
    char crlf[2] = {'\r', '\n'}; 
    int i;
    int ret = 0;
    ssize_t bytesRead, bytesWrite, bytes2Shell;
    /* Struct used for polling */
    struct pollfd fds[2] =  
    {
        {STDIN_FILENO, POLLIN, 0},
        {shellToTerminal[0], POLLIN, 0}
    };

    if(shellOn) {
        while (1) {
            if(poll(fds, 2, 0) == -1) {
                printError(errno);
            }
	    if(ret == 1) {
                /*Condition is true if shell gets EOF*/
                exitTerminal(0);
            }
            if((fds[0].revents & POLLIN) && (ret == 0)) {
                /*Read in input from stdin.
                Read in a large buffer in case multiple characters 
                are written in at once */
                bytesRead = read(STDIN_FILENO, buf, BUFFER_SIZE);
                if (bytesRead == -1) {
                    printError(errno);
                }
                ret = readTerminal(buf, bytesRead);
                /*Value of ret is 1 if ^D is received. Otherwise 0*/
            }
            if (fds[1].revents & POLLIN) {
                /*Read in input from the shell output*/
	            bytesRead = read(shellToTerminal[0], buf, BUFFER_SIZE);
                if (bytesRead == -1) {
                    printError(errno);
                }
		        readShell(buf, bytesRead);
            }
            
            /*Handles cases when poll reports that the shell/terminal processes closed */
            if (fds[0].revents & (POLLERR+POLLHUP)) {
	      //fprintf(stderr, "Received EOF from terminal\r\n");
                exitTerminal(0);
            }
            if (fds[1].revents & (POLLERR+POLLHUP)) {
	      //fprintf(stderr, "Received EOF from shell\r\n");
                exitTerminal(0);
            }
        }
    }
    else {
        /*Case if '--shell' is not specified*/
        while(1) {
            /*Read in a large buffer in case multiple characters 
            are written in at once */
            bytesRead = read(STDIN_FILENO, buf, BUFFER_SIZE);
            if (bytesRead == -1) {
                printError(errno);
            }
            readTerminal(buf, bytesRead);
        }
    }
}

int main(int argc, char* argv[]) {
    int c, d, status;
    int* ptr;
    static struct option commands[] =
    {
        {"shell", no_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    c = getopt_long(argc, argv, "s", commands, ptr);
    switch (c) {
        case 's':
            shellOn = 1;
            break;
        case -1:
            /*In this case there were no arguments*/
            break;
        default:
            fprintf(stderr, "Usage: lab1a [shell]\n");
            exit(1);
    }
    changeTerminal();
    if (shellOn) {
        c = pipe(terminalToShell);
        d = pipe(shellToTerminal);
        if (c == -1 || d == -1){
            printError(errno);
        }
        //signal to catch sigpipe
        signal(SIGPIPE, signal_handler);
        /*Start of the child process*/
        childPID = fork();
        if (childPID == -1) {
            printError(errno);
        }
        if (childPID == 0) {
            if(close(0) == -1)
                printError(errno); 
            if(dup(terminalToShell[0]) == -1)
                printError(errno); 
            if(close(terminalToShell[0]) == -1)
                printError(errno); 
            if(close(1) == -1)
                printError(errno);
            if(dup(shellToTerminal[1]) == -1)
                printError(errno);
            if(close(2) == -1)
                printError(errno);
            if(dup(shellToTerminal[1]) == -1)
                printError(errno);
            if(close(shellToTerminal[1]) == -1)
                printError(errno);
	        if(close(shellToTerminal[0]) == -1)
                printError(errno);
	        if(close(terminalToShell[1]) == -1)
                printError(errno);
            status = execvp("/bin/bash", NULL);
            if(status == -1)
                printError(errno);
        }
        else {
            if(close(terminalToShell[0]) == -1)
                printError(errno);
            if(close(shellToTerminal[1]) == -1)
                printError(errno);
	       readWrite();
	   }
    }
    /*If --shell is not called*/
    else 
        readWrite();
}
