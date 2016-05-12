#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "hash_table.h"

const int MAX_USER_NAME_SIZE = 250;
const int MAX_CARD_NAME_SIZE = 256;
const int MAX_USER_NUM = 10;
const int MAX_LINE_SIZE = 500;
const int PIN_SIZE = 4;


Bank* bank_create()
{
    Bank *bank = (Bank*) malloc(sizeof(Bank));
    if(bank == NULL)
    {
        perror("Could not allocate Bank");
        exit(1);
    }

    // Set up the network state
    bank->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&bank->rtr_addr,sizeof(bank->rtr_addr));
    bank->rtr_addr.sin_family = AF_INET;
    bank->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&bank->bank_addr, sizeof(bank->bank_addr));
    bank->bank_addr.sin_family = AF_INET;
    bank->bank_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->bank_addr.sin_port = htons(BANK_PORT);
    bind(bank->sockfd,(struct sockaddr *)&bank->bank_addr,sizeof(bank->bank_addr));

    // Set up the protocol state
    // TODO set up more, as needed
    bank->logged_user_ht = hash_table_create(MAX_USER_NUM);
    bank->user_balance_ht = hash_table_create(MAX_USER_NUM);

    return bank;
}

void bank_free(Bank *bank)
{
    if(bank != NULL)
    {
        close(bank->sockfd);
        free(bank);
    }
}

ssize_t bank_send(Bank *bank, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(bank->sockfd, data, data_len, 0,
                  (struct sockaddr*) &bank->rtr_addr, sizeof(bank->rtr_addr));
}

ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len)
{
    // Returns the number of bytes received; negative on error
    return recvfrom(bank->sockfd, data, max_data_len, 0, NULL, NULL);
}
/* check if in range a-z A-Z and NULL*/
int check_username(char* username) {
	int i;
	if (username == NULL) {
		return 1;
	}
	//TODO: potential bufferoverflow
	int len = strlen(username);
	int flag;
	for (i = 0; (i < len); i++) {
		if ((username[i] >= 'a' && username[i] <= 'z') ||
				(username[i] >= 'A' && username[i] <= 'Z')) {
		} else {
			return 1;		
		}
	} 
	return 0;

}

/* [0-9][0-9][0-9][0-9] */
int check_pin(char* pin) {    
    int len = strlen(pin); //TODO: maybe overflow?
    if (len != 4) {
        return 1;
    }

	 int i;
    for (i = 0; i < len; i++) {
        if (!(pin[i] >= '0' && pin[i] <= '9')) {
				return 1;            
        }
    }
    return 0;
}

/* NOT yet check negative or positive*/
int check_balance(char* args) {
    int len = strlen(args);
	 int i;
    for (i = 0; i < len; ++i) {
        if (!(args[i] >= '0' && args[i] <= '9')) {
     			return 1;       
        }
    }
    return 0;
}
void bank_create_user(Bank* bank, char* args1, char* args2, char* args3) {
	FILE *fp;
	char card_name[MAX_CARD_NAME_SIZE + 1];
	
	if (!args1 || !args2 || !args3) {
      printf("Usage: create-user <user-name> <pin> <balance>\n");
		return;		
	}
	
	if (check_username(arg1) != 0 || check_pin(arg2) != 0 
		|| check_balance(arg3) != 0) {
   	printf("Usage: create-user <user-name> <pin> <balance>\n");
		return;	
	} 
	 
   if ((char*)hash_table_find(bank->user_balance_ht, args1) != NULL) {
       printf("Error: user %s already exists\n", args1);
       return;
   }

	memset(card_name, 0, MAX_CARD_NAME_SIZE);
   strcat(card_name, args1);
   strcat(card_name, ".card");

   fp = fopen(card_file_name, "w");
   if (!fp) {
       printf("Error creating card file for user %s\n", args1);
       return;
   }

   fprintf(fp, "%s\n", args1);
   fprintf(fp, "%s\n", args2);
   fclose(fp);

   tmp = (char*)malloc(64);
   strcpy(tmp, args3);
   hash_table_add(bank->user_balance_ht, args1, tmp);

   printf("Created user %s\n", args1);

}
void handle_deposit(Bank* bank, char* args1, char* args2) {
    int ret = 0;
    int amt = 0;
    int balance = 0;
    char *tmp;
    char card_file_name[MAX_CARD_FILE_NAME_SIZE + 1];

    if (args1 == NULL || args2 == NULL) {
        printf("Usage: deposit <user-name> <amt>\n");
        return;
    }

    ret = check_username(args1); 
    if (ret != 0) {
        printf("Usage: deposit <user-name> <amt>\n");
        return;
    }

    ret = check_amt(args2);
    if (ret != 0) {
        printf("Usage: deposit <user-name> <amt>\n");
        return;
    }
    sscanf(args2, "%d", &amt);

    if ((char*)hash_table_find(bank->user_balance_ht, args1) == NULL) {
        printf("No such user\n");
        return;
    }

    memset(card_file_name, 0, MAX_CARD_FILE_NAME_SIZE);
    strcat(card_file_name, args1);
    strcat(card_file_name, ".card");

    sscanf((char*)hash_table_find(bank->user_balance_ht, args1), "%d", &balance);

    if (balance + amt < 0) {
        printf("Too rich for this program\n");
        return;
    }

    balance += amt;
    tmp = (char*)malloc(64);
    sprintf(tmp, "%d", balance);
    hash_table_del(bank->user_balance_ht, args1);
    hash_table_add(bank->user_balance_ht, args1, tmp);

    printf("$%d added to %s's account\n", amt, args1);
}

void handle_balance(Bank* bank, char* args) {
    int ret = 0;
    char* balance;

    if (args == NULL) {
        printf("Usage: balance <user-name>\n");
        return;
    }

    ret = check_username(args);
    if (ret != 0) {
        printf("Usage: balance <user-name>\n");
        return;
    }

    balance = (char*)hash_table_find(bank->user_balance_ht, args);
    if (balance == NULL) {
        printf("No such user\n");
        return;
    }

    printf("$%s\n", balance);
    return;
}

void bank_process_local_command(Bank *bank, char *command, size_t len)
{
    // TODO: Implement the bank's local commands
	char com[50];
	char arg1[1024];
	char arg2[1024];
	char arg3[1024];
	
	sscanf(command, "%s%s%s%s", com, arg1, arg2, arg3);

	if (strcmp(com, "create-user") == 0) {
		bank_create_user(bank, arg1, arg2, arg3);	
	} 
	else if (strcmp(com, "deposit") == 0) {
		bank_deposit(bank, arg1, arg2);	
	} 
	else if (strcmp(com, "balance") == 0) {
		bank_balance(bank, arg1);	
	}
	else {
		printf("Invalid command\n");	
	}
}

void bank_process_remote_command(Bank *bank, char *command, size_t len)
{
    // TODO: Implement the bank side of the ATM-bank protocol

	/*
	 * The following is a toy example that simply receives a
	 * string from the ATM, prepends "Bank got: " and echoes 
	 * it back to the ATM before printing it to stdout.
	 */

	/*
    char sendline[1000];
    command[len]=0;
    sprintf(sendline, "Bank got: %s", command);
    bank_send(bank, sendline, strlen(sendline));
    printf("Received the following:\n");
    fputs(command, stdout);
	*/
}
