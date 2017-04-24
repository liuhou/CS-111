// NAME: Jeffrey Chan
// EMAIL: jeffschan97@gmail.com
// ID: 004611638

#include<stdio.h>
#include<getopt.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<poll.h>
#include<signal.h>
#include<fcntl.h>
#include<netdb.h>
#include<netinet/in.h>
#include<sys/wait.h>
#include<sys/types.h>

// buffer to hold read characters
int BUFFER_SIZE = 1024;
char extraBuffer[1024];

// int arrays to hold pipes between processes
int pipeToChild[2];
int pipeToParent[2];

pid_t pid; // process id for when forking process
int socketFD, newSocketFD;

void createPipe(int pipeHolder[2]) {
    if (pipe(pipeHolder) == -1) {
	fprintf(stderr, "error: error in creating pipe");
	exit(1);
    }
}

void execShell() {
    // pipe reconnection
    close(pipeToChild[1]);
    close(pipeToParent[0]);
    dup2(pipeToChild[0], STDIN_FILENO);
    dup2(pipeToParent[1], STDOUT_FILENO);
    close(pipeToChild[0]);
    close(pipeToParent[1]);
    
    // format arguments for shell
    char *execvp_argv[2];
    char execvp_filename[] = "/bin/bash";
    execvp_argv[0] = execvp_filename;
    execvp_argv[1] = NULL;

    // execute shell with it's filename as an argument
    if (execvp(execvp_filename, execvp_argv) == -1) {
        fprintf(stderr, "error: error in executing shell");
        exit(1);
    }
}

void readWrite(int socketFD) {
    char buffer;
    while (read(socketFD, &buffer, sizeof(char))) {
        write(socketFD, &buffer, sizeof(char));
    }
}

int main(int argc, char *argv[]) {
     int option = 0; // used to hold option
     int portNumber, clientLength;
     struct sockaddr_in serverAddress, clientAddress;

    // arguments that this program recognizes
    static struct option options[] = {
    	{"port", required_argument, 0, 'p'},
        {"encrypt", required_argument, 0, 'e'},
        {0, 0, 0, 0}
    };

    // parse through arguments
    while ((option = getopt_long(argc, argv, "p:e", options, NULL)) != -1) {
        switch (option) {
            case 'p':
                portNumber = atoi(optarg);
                break;
            case 'e':
                printf("received encrypt option\n");
                break;
            default:
                fprintf(stderr, "error: unrecognized argument\nrecognized arguments:\n--port\n--encryp\n");
                exit(1);
        }
    }

    // setup socket settings
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) { fprintf(stderr, "error: error in opening socket\n"); exit(1); }
    memset((char*) &serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // bind host address
    if (bind(socketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "error: error in binding socket");
        exit(1);
    }

    // listen for incoming clients
    listen(socketFD, 5);
    clientLength = sizeof(clientAddress);

    // accept connection from client
    newSocketFD = accept(socketFD, (struct sockaddr *) &clientAddress, &clientLength);
    if (newSocketFD < 0) {
	fprintf(stderr, "error: error accepting client");
	exit(1);
    }

    // create pipes
    createPipe(pipeToChild);
    createPipe(pipeToParent);

    // fork process and exec shell
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "error: error in forking\n");
        exit(1);
    }
    else if (pid == 0) {
        // CHILD PROCESS
        execShell();
    }
    else {
        // PARENT PROCESS
        readWrite(newSocketFD);
    }

    exit(0);
}