#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/stat.h>
#include <errno.h>

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

int validSize(char * fText, char * fKey) {

	struct stat st;
	off_t textSize, keySize;

	if (stat(fText, &st) == 0) {
		textSize = st.st_size;
	//	printf("textSize: %jd\n", (intmax_t)textSize);
	}

	else {
		fprintf(stderr, "size error on %s: %s\n", fText, strerror(errno));
	}

	if (stat(fKey, &st) == 0) {
		keySize = st.st_size;
	//	printf("keySize: %jd\n", (intmax_t)keySize);
	}
	
	else {
		fprintf(stderr, "size error on %s: %s\n", fKey, strerror(errno));
	}

	if (keySize >= textSize) 
		return 1;

	else
		return 0;

	
}

int createMessage(char * message, size_t length, char *argv[]) {

	int returnVal, i, ascii;
	FILE *fp;
	char buff1[100000];
	char buff2[100000];

	/* Create the plain text string */
	fp = fopen(argv[1], "r");
	fgets(buff1, 100000, fp);
//	printf("buff1: %s", buff1);
	fclose(fp);

	/* Create the key string */
	fp = fopen(argv[2], "r");
	fgets(buff2, 100000, fp);
//	printf("buff2: %s", buff2);
	fclose(fp);

	/* Remove newline character from each string */
	buff1[strcspn(buff1, "\n")] = '\0';
	buff2[strcspn(buff2, "\n")] = '\0';

	/* Check plaintext string for bad characters */
	for (i = 0; i < strlen(buff1); i++) {
		ascii = buff1[i];
		if (ascii < 65) {
			if (ascii != 32) {
				return 2;
			}
		}
		else if (ascii > 90) {
			return 2;
		}

	}
	/* Create the message composed of the two strings */
	returnVal = snprintf(message, length, "dec$$%s$$%s##", buff1 , buff2);

//	printf("Message for server is: %s\n", message);
	/* SnPrintf was unsuccessful, return failure */	
	if (returnVal < 0) {return 1;}

	/* Sucess */
	return 0;

}

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[150000];
 	int returnVal;

	/* Check usage and args */   
	if (argc != 4) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); exit(1); } 

	/* Set up the server address struct */
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); //clear 
	portNumber = atoi(argv[3]); // Convert string port to int
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert name to special address

	/* Error getting host by name */
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(1); }

	/* Copy in the address */
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

	/* Set up the socket */
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	/* Connect to server */
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	/* Validate that plaintext length > keylength */
	if (validSize(argv[1], argv[2])) {
//		printf("validated\n");
	}
	else {
		fprintf(stderr, "Error: Key file may not be shorter than text file\n");
		return 1;
	}

	// Get input message from user
/*	printf("CLIENT: Enter text to send to the server, and then hit enter: ");
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	fgets(buffer, sizeof(buffer) - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
*/

	/* Create input message from files */
	memset(buffer, '\0', sizeof(buffer));
	returnVal = createMessage(buffer, sizeof(buffer), argv);
	
	if (returnVal != 0) {
		if (returnVal == 1) {
			fprintf(stderr, "createMessage error\n");
		}	
		else if (returnVal == 2) {
			fprintf(stderr, "createMessage error: invalid characters\n");
		}
		return 1;
	}
/*	else {
		printf("buffer: %s\n", buffer);
	}
*/
	/* Send message to server */
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");

//	printf("charsWritten: %d strelen(buffer): %d\n", charsWritten, strlen(buffer));
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	/* Get return message from server */
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	
	/* Error reading message from server */
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	/* Print message from server */
	printf("%s\n", buffer);

	close(socketFD);

	return 0;
}
