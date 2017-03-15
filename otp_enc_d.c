#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>


void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

void childMethod(int connectionFD) {
	int charsRead;
	char buffer[256];
	char temp[256];
	char * token;
	pid_t spawnPid = -5;

	spawnPid = fork();

	if (spawnPid == -1) {
		perror("Hull breach!");
	}

	/* inside child process */
	else if (spawnPid == 0) {

	/* Get the message from the client and display it */
	memset(buffer, '\0', 256);
	charsRead = recv(connectionFD, buffer, 255, 0); // Read msg
	if (charsRead < 0) error("ERROR reading from socket");
	printf("SERVER: I received this from the client: \"%s\"\n", buffer);

	/* Verify that client is otp_enc */
	strcpy(temp, buffer);

	printf("SERVER: temp is: \"%s\"\n", temp);


	// Send a Success message back to the client
	charsRead = send(connectionFD, "I am the server, and I got your message", 39, 0); // Send success back
	if (charsRead < 0) error("ERROR writing to socket");

	close(connectionFD); 

	exit(0);
	}
}

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber; //, charsRead;
	socklen_t sizeOfClientInfo;
//	char buffer[256];
	struct sockaddr_in serverAddress, clientAddress;

	/* Verify usage is correct */
	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } 

	/* Set up the address struct for this process (the server) */
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); 
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	/* Set up the socket */
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections


	/* Create signal handler to reap zombies asynchronously */
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGCHLD, &sa, 0) == -1) {
		perror(0);
		exit(1);
	}

	/* Enter loop to accept connections and create child processes */
	while(1) {
	// Accept a connection, blocking if one is not available until one connects
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
	establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
	if (establishedConnectionFD < 0) error("ERROR on accept");

	/* Receive message and encrypt in a child process */
	childMethod(establishedConnectionFD);

}
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
