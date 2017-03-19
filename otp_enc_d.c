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

/* Source citation: 
 * 	Error handling and socket code based on Client.c and Server.c code 
 * 	provided in CS344 lecture slides (Module 4 by Benjamin Brewster
 */ 

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

/* Function thhat receives a cipher and key to encode and save as 'cipher' string */
void encode(char * text, char * key, char * cipher) {
	int i;
	int textChar, keyChar, cipherChar;

	//memset(cipher,'\0', sizeof(cipher)); 

	/* Iterate through text to cipher */
	for (i=0; i < strlen(text); i++) {

		/* First, convert ascii A-Z and space to 0 through 27 */
		if (text[i] == ' ') { textChar = 0; } // Space is not related to its ascii value
		else { textChar = text[i] - 64; }
	
		if (key[i] == ' ') { keyChar = 0; } // Space is not related to its ascii value
		else { keyChar = key[i] - 64; }

		/* Addition: textChar + keyChar */
		cipherChar = textChar + keyChar;  //Adding key to cipher gets non-ascii cipher value


		/* Ensure new cipher Char is between 0 and 27 */
		if (cipherChar >= 27) { cipherChar -= 27; }

		/* Undo conversion of ascii */
		if (cipherChar == 0) { cipher[i] = ' '; }
		else { cipher[i] = cipherChar + 64; }		
	}

	/* cipher string now contains encoded message */
}

void childMethod(int connectionFD) {
	int charsRead;
	char completeMessage[150000], readBuffer[1000];
	char temp[150000];
	char text[100000];
	char key[100000];
	char cipher[100000];
	char * type = "enc";
	char * token;
	pid_t spawnPid = -5;

	spawnPid = fork();

	if (spawnPid == -1) { perror("Hull breach!"); }

	/* inside child process */
	else if (spawnPid == 0) {

		/* Clear out message buffer */
		memset(completeMessage, '\0', sizeof(completeMessage));

		/* Keep reading until all data is received */
		while (strstr(completeMessage, "##") == NULL) {
			memset(readBuffer, '\0', sizeof(readBuffer)); // Clear read buffer
			
			/* Read from socket up to the size of the readBuffer -1 to preserve '\0' */
			charsRead = recv(connectionFD, readBuffer, sizeof(readBuffer) - 1, 0);
		
			/* Add current buffer to complete message string */
			strcat(completeMessage, readBuffer);

			/* Escape while-loop if error reading from socket */
			if (charsRead == -1) { printf("r == -1\n"); break;}
			if (charsRead == 0) { printf("r == 0\n"); break; }
	}

	/* Remove terminal flag from message received by server (ie. remove '##') */
	int terminalLoc = strstr(completeMessage, "##") - completeMessage;
	completeMessage[terminalLoc] = '\0';

	/* Verify that client is otp_enc before encoding message*/

	if (strncmp(completeMessage, type, 3) != 0) {
		fprintf(stderr,"OTP_ENC_D: Client type must be encoder\n");	
		/* Send blank message back to client, indicating failed encoding */
		charsRead = send(connectionFD, " ",1,0 );	
		if (charsRead < 0) error("OTP_ENC_D: Error writing to socket\n");

	}	

	/* Client is encoder */
	else {
		
		memset(temp, '\0', sizeof(temp));
		strcpy(temp, completeMessage);

		/* Clear contents of key and text strings */
		memset(key, '\0', sizeof(key));
		memset(text, '\0', sizeof(text));
		memset(cipher, '\0', sizeof(cipher));

		/* Remove "enc" text */
		token = strtok(temp, "$$");

		/* Remove plaintext text */
		token = strtok(NULL, "$$");

		/* Save plaintext text */
		strcpy(text, token);


		/* Remove key text */
		token = strtok(NULL, "$$");
	
		/* Save key text */
		strcpy(key, token);

		/* Pass key and text to cipher function and return encoded messaged saved in cipher string */
		encode(text, key, cipher);

		/* Send a Success message back to the client */
		charsRead = send(connectionFD, cipher, strlen(cipher), 0); // Send success back
		if (charsRead < 0) error("ERROR writing to socket\n");

	}

	/* Close connection */
	close(connectionFD); 
	
	/* Exit child process on success */
	exit(0);
	}
}

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber; //, charsRead;
	socklen_t sizeOfClientInfo;
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
	listen(listenSocketFD, 5); // Socket can now receive up to 5 connections


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
		/* Accept a connection, blocking if one is not available until one connects */
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

		/* Receive message and encrypt in a child process */
		childMethod(establishedConnectionFD);

}
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
