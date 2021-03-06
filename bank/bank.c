#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "hash_table.h"
#include <errno.h>
#include <limits.h>

const int MAX_USER_NAME_SIZE = 250;
const int MAX_CARD_NAME_SIZE = 256;
const int MAX_LINE_SIZE = 500;
const int PIN_SIZE = 4;
const int MAX_USER_NUM = 100;


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

/* TODO:not check negative or positive*/
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
	if (!args1 || !args2 || !args3) {
      printf("Usage: create-user <user-name> <pin> <balance>\n");
		return;		
	}
	
	if (check_username(args1) != 0 || check_pin(args2) != 0 
		|| check_balance(args3) != 0) {
   	printf("Usage: create-user <user-name> <pin> <balance>\n");
		return;	
	} 
	 
   if ((char*)hash_table_find(bank->user_balance_ht, args1) != NULL) {
       printf("Error: user %s already exists\n", args1);
       return;
   }

	FILE *fp;
	char card_name[MAX_CARD_NAME_SIZE + 1];
	
	memset(card_name, 0, MAX_CARD_NAME_SIZE);
   strcat(card_name, args1);
   strcat(card_name, ".card");

   fp = fopen(card_name, "w");
   if (!fp) {
       printf("Error creating card file for user %s\n", args1);
       return;
   }

	printf("Created user %s\n", args1);

	/* write to card*/
   fprintf(fp, "%s\n", args1);
   fprintf(fp, "%s\n", args2);
   fclose(fp);

 	char* tmp;
   tmp = (char*)malloc(64);
	int len = strlen(args3);
	if (len > 64) {
		printf("Usage: create-user <user-name> <pin> <balance>\n");
		return;
	}
   strncpy(tmp, args3, len);
   hash_table_add(bank->user_balance_ht, args1, tmp);
}

/* check integer overflow and*/
int check_amt(char* args) {
   int i;
   int len = strlen(args);
   for (i = 0; i < len; ++i) {
       if (!(args[i] >= '0' && args[i] <= '9')) {
          return 1;
       }
   }
	
	// TODO: check integer overflow
	
	
   return 0;
}

void bank_deposit(Bank* bank, char* name, char* amt) {
    if (!name || !amt) {
        printf("Usage: deposit <user-name> <amt>\n");
        return;
    }
   if (check_username(name) || check_amt(amt)) {
        printf("Usage: deposit <user-name> <amt>\n");
        return;
    }
   if (!hash_table_find(bank->user_balance_ht, name)) {
       printf("No such user\n");
       return;
   }
	
	// handle name
	char card_name[MAX_CARD_NAME_SIZE + 1];
   memset(card_name, 0, MAX_CARD_NAME_SIZE);
   strcat(card_name, name);
   strcat(card_name, ".card");


	// handle balance
	int amount = 0; 
	int balance = 0;   
	sscanf(amt, "%d", &amount);		
   sscanf((char*)hash_table_find(bank->user_balance_ht, name), "%d", &balance);

    if (balance + amount < 0) {
        printf("Too rich for this program\n");
        return;
    }

    balance += amount;
	 char* tmp;
    tmp = (char*)malloc(64);
    sprintf(tmp, "%d", balance);
    hash_table_del(bank->user_balance_ht, name);
    hash_table_add(bank->user_balance_ht, name, tmp);

    printf("$%d added to %s's account\n", amount, name);
}


void bank_balance(Bank* bank, char* name) {
   if (!name || check_username(name)) {
       printf("Usage: balance <user-name>\n");
       return;
   }

   char* balance;
   balance = (char*)hash_table_find(bank->user_balance_ht, name);
   if (!balance) {
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

void bank_encrypt(Bank* bank, char* str, int len) {
	unsigned char key[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC};
	int i;	
	for (i = 0; i < len; i++) {
		str[i] ^= key[i];		
	}
}

void bank_decrypt(Bank *bank, char* str, int len){
	unsigned char key[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC};
	int i;	
	for (i = 0; i < len; i++) {
		str[i] ^= key[i];		
	}
}

void handle_balance_atm(Bank* bank, char* args) {
    char* tmp;
    char send_msg[MAX_LINE_SIZE + 1];
    memset(send_msg, 0, MAX_LINE_SIZE);
    
    tmp = (char*)hash_table_find(bank->user_balance_ht, args);
    printf("balance:%s\n", tmp);
    strcat(send_msg, tmp);
    
    bank_encrypt(bank, send_msg, strlen(send_msg));
    bank_send(bank, send_msg, strlen(send_msg));
}


void handle_user_logout(Bank* bank, char* args) {
    char send_msg[MAX_LINE_SIZE + 1];
    memset(send_msg, 0, MAX_LINE_SIZE);

    if ((char*)hash_table_find(bank->logged_user_ht, args) == NULL)  {
        strcat(send_msg, "No user logged in");
    } else {
        strcat(send_msg, "OK");
        hash_table_del(bank->logged_user_ht, args);
    }

    bank_encrypt(bank, send_msg, strlen(send_msg));
    bank_send(bank, send_msg, strlen(send_msg));
    return;
}

void handle_user_login(Bank* bank, char* args) {
    char send_msg[MAX_LINE_SIZE + 1];
    memset(send_msg, 0, MAX_LINE_SIZE);

    if ((char*)hash_table_find(bank->user_balance_ht, args) == NULL ||
            (char*)hash_table_find(bank->logged_user_ht, args) != NULL)  {
        strcat(send_msg, "NA");
    } else {
        strcat(send_msg, "OK");
        hash_table_add(bank->logged_user_ht, args, "1");
    }

    bank_encrypt(bank, send_msg, strlen(send_msg));
    bank_send(bank, send_msg, strlen(send_msg));
}


void handle_withdraw(Bank* bank, char* args1, char* args2) {
    char* tmp;
    int balance;
    int amt;
    char send_msg[MAX_LINE_SIZE + 1];
    memset(send_msg, 0, MAX_LINE_SIZE);
    
    tmp = (char*)hash_table_find(bank->user_balance_ht, args1);
    sscanf(tmp, "%d", &balance);
    sscanf(args2, "%d", &amt);
    if (balance < amt) {
        strcat(send_msg, "IF");
    } else {
        strcat(send_msg, "AD");
        balance -= amt;
        memset(tmp, 0, 64);
        sprintf(tmp, "%d", balance);
        hash_table_add(bank->user_balance_ht, args1, tmp);
    }

    bank_encrypt(bank, send_msg, strlen(send_msg));
    bank_send(bank, send_msg, strlen(send_msg));
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

    char cmd[64];
    char args1[2048];
    char args2[2048];

    bank_decrypt(bank, command, strlen(command));    
    sscanf(command, "%s%s%s", cmd, args1, args2);

    if (strcmp("user-login", cmd) == 0) {
        handle_user_login(bank, args1);
    } else if (strcmp("withdraw", cmd) == 0) {
        handle_withdraw(bank, args1, args2);
    } else if (strcmp("balance", cmd) == 0) {
        handle_balance_atm(bank, args1);
    } else if (strcmp("user-logout", cmd) == 0) {
        handle_user_logout(bank, args1);
    }

}



