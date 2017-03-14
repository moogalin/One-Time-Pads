#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>


int main(int argc, char *argv[]) {

	const int max_ascii = 91;
	const int min_ascii = 65;
	int total_ascii;
	int rand_ascii;

	/* Verify that there is only one command-line argument */
	if (argc != 2) { fprintf(stderr,"USAGE: %s keylength\n", argv[0]); exit(1); }

	/* Get key length from command arguments */
	total_ascii = atoi(argv[1]);

	/* Seed random numbers */
	srand(time(NULL));

	/* Generate random ascii characters (A-Z) up to total_ascii */
	while (total_ascii) {
	
		rand_ascii = rand() % (max_ascii + 1 - min_ascii) + min_ascii;
		
		if (rand_ascii == 91) { printf(" "); }

		else { printf("%c", rand_ascii); }
	
		total_ascii--;
	}
	printf("\n");

	return 0;
}
