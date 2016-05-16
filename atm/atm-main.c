/* 
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <stdlib.h>

static const char prompt[] = "ATM: ";

int main(int argc, char** argv)
{
    char user_input[1000];

    ATM *atm = atm_create();

	FILE* fp;
	if (argc != 2) {
		printf("Usage: atm <init-filename>.atm\n");
		return 64;
	}
	
	fp = fopen(argv[1], "r");
	if (!fp) {
		printf("Error opening ATM initialization file\n");
		return 64;
	}

    printf("%s", prompt);
    fflush(stdout);

    while (fgets(user_input, 10000,stdin) != NULL)
    {
        atm_process_command(atm, user_input);
        fflush(stdout);
		if (atm->open_session == 1) {
			printf("ATM (%s): ", atm->username);
		} else {
			printf("%s", prompt);		
		}
		
    }
	return EXIT_SUCCESS;
}
