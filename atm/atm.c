#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_USER_NUM 100
#define MAX_LINE_SIZE 1000
#define MAX_USER_NAME_SIZE 250
#define PIN_SIZE 4
#define MAX_CARD_NAME_SIZE 256

void atm_encrypt(ATM *atm, char *str, int len) {
	unsigned char key[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC};
	int i;	
	for (i = 0; i < len; i++) {
		str[i] ^= key[i];		
	}

}

void atm_decrypt(ATM *atm, char *str, int len) {
	unsigned char key[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC};
	int i;	
	for (i = 0; i < len; i++) {
		str[i] ^= key[i];		
	}
}

ATM* atm_create()
{
    ATM *atm = (ATM*) malloc(sizeof(ATM));
    if(atm == NULL)
    {
        perror("Could not allocate ATM");
        exit(1);
    }

    // Set up the network state
    atm->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&atm->rtr_addr,sizeof(atm->rtr_addr));
    atm->rtr_addr.sin_family = AF_INET;
    atm->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&atm->atm_addr, sizeof(atm->atm_addr));
    atm->atm_addr.sin_family = AF_INET;
    atm->atm_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->atm_addr.sin_port = htons(ATM_PORT);
    bind(atm->sockfd,(struct sockaddr *)&atm->atm_addr,sizeof(atm->atm_addr));

    // Set up the protocol state
    // TODO set up more, as needed
	atm->open_session = 0;

    return atm;
}

void atm_free(ATM *atm)
{
    if(atm != NULL)
    {
        close(atm->sockfd);
        free(atm);
    }
}

ssize_t atm_send(ATM *atm, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(atm->sockfd, data, data_len, 0,
                  (struct sockaddr*) &atm->rtr_addr, sizeof(atm->rtr_addr));
}

ssize_t atm_recv(ATM *atm, char *data, size_t max_data_len)
{
    // Returns the number of bytes received; negative on error
    return recvfrom(atm->sockfd, data, max_data_len, 0, NULL, NULL);
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


void atm_begin_session(char* arg, ATM* atm) {
    
    char card_name[MAX_CARD_NAME_SIZE + 1];
    char card_pin[PIN_SIZE + 1];
    if (atm->open_session != 0) {
        printf("A user is already logged in\n");
        return;
    }

	// secure username input
    if (check_username(arg) != 0) {
        printf("Usage: begin-session <user-name>\n");
        return;
    }

	// Find card
    strncpy(card_name, arg, MAX_CARD_NAME_SIZE - 4);
    strcat(card_name, ".card");
	FILE* fp;
    fp = fopen(card_name, "r");
    if (!fp) {
        printf("No such user\n");
        return;
    }

	// get pin from input
	char input_pin[1024];
	printf("PIN? ");
	fgets(input_pin, 1024+1, stdin);
	int len;
	len = strlen(input_pin);
	if (input_pin[len - 1] == '\n') {
		input_pin[len -1] = '\0';
		len--;
	}	
	if (strlen(input_pin) != PIN_SIZE) {
		printf("Not authorized\n");
		return;
	}
	char pin[PIN_SIZE + 1];
	memset(pin, 0, PIN_SIZE + 1);
	strncpy(pin, input_pin, PIN_SIZE);
	
	// check pin
	char name_on_card[MAX_USER_NAME_SIZE + 1];
    fgets(name_on_card, MAX_USER_NAME_SIZE + 1, fp);
    len = strlen(name_on_card);
	//TODO: not nesssary
    if (name_on_card[len - 1] == '\n') {
        name_on_card[len - 1] = '\0';
    }

	char pin_on_card[PIN_SIZE + 1];
    fgets(pin_on_card, PIN_SIZE + 1, fp);    
	len = strlen(pin_on_card);
    if (card_pin[len - 1] == '\n') {
        card_pin[len - 1] = '\0';
    }
    fclose(fp);

    if (strncmp(name_on_card, arg, MAX_USER_NAME_SIZE) != 0 || strncmp(pin_on_card, pin, PIN_SIZE) != 0) {
        printf("Not authorized\n");
        return;
    }

	char send_msg[MAX_LINE_SIZE + 1];
    char recv_msg[MAX_LINE_SIZE + 1];
    memset(send_msg, 0, MAX_LINE_SIZE);
    memset(recv_msg, 0, MAX_LINE_SIZE);
    strcat(send_msg, "user-login ");
    strcat(send_msg, arg);

    atm_encrypt(atm, send_msg, strlen(send_msg));
    atm_send(atm, send_msg, MAX_LINE_SIZE);
    atm_recv(atm, recv_msg, MAX_LINE_SIZE);
    atm_decrypt(atm, recv_msg, strlen(recv_msg));

    // not authorized
    if (strncmp("NA", recv_msg, strlen("NA")) == 0) {
        printf("Not authorized\n");
        return;
    }

    atm->open_session = 1;
    memset(atm->username, 0, MAX_USER_NAME_SIZE + 1);
    strncpy(atm->username, arg, MAX_USER_NAME_SIZE);

    printf("Authorized\n");
    
    return;
}

// return 0 if ok 
int check_amt(char* args) {
   int i;
   int len = strlen(args);
   for (i = 0; i < len; ++i) {
       if (!(args[i] >= '0' && args[i] <= '9')) {
          return 1;
       }
   }
   return 0;
}

void atm_withdraw (char* arg, ATM* atm) {
	if (atm->open_session != 1) {
		printf("No user logged in\n");
		return;
	} 
	
	if (check_amt(arg) != 0) {
		printf("Usage: withdraw <amt>\n");
		return;
	}
	
	int int_amt;
	sscanf(arg, "%d", &int_amt);

	char msg_out[MAX_LINE_SIZE + 1];
	char msg_in[MAX_LINE_SIZE + 1];

	printf("123");
	
    memset(msg_out, 0, MAX_LINE_SIZE);
    memset(msg_in, 0, MAX_LINE_SIZE);
    
	strcat(msg_out, "withdraw ");
    strcat(msg_out, atm->username);
    strcat(msg_out, " ");
    strcat(msg_out, arg);

    atm_encrypt(atm, msg_out, strlen(msg_out));
    atm_send(atm, msg_out, MAX_LINE_SIZE);
    atm_recv(atm, msg_in, MAX_LINE_SIZE);
    atm_decrypt(atm, msg_in, strlen(msg_in));
    
    // insufficient funds
    if (strncmp("IF", msg_in, strlen("IF")) == 0) {
        printf("Insufficient funds\n");
        return;
    }

    // amt dispensed
    if (strncmp("AD", msg_in, strlen("AD")) == 0) {
        printf("$%d dispensed\n", int_amt);

    }
}


void atm_balance(ATM* atm) {
    char send_msg[MAX_LINE_SIZE + 1];
    char recv_msg[MAX_LINE_SIZE + 1];

    if (atm->open_session == 0) {
        printf("No user logged in\n");
        return;
    }
    memset(send_msg, 0, MAX_LINE_SIZE);
    memset(recv_msg, 0, MAX_LINE_SIZE);
    strcat(send_msg, "balance ");
    strcat(send_msg, atm->username);

    atm_encrypt(atm, send_msg, strlen(send_msg));
    atm_send(atm, send_msg, MAX_LINE_SIZE);
    atm_recv(atm, recv_msg, MAX_LINE_SIZE);
    atm_decrypt(atm, recv_msg, strlen(recv_msg));

    return;
}

void atm_end_session(ATM* atm) {
    char send_msg[MAX_LINE_SIZE + 1];
    char recv_msg[MAX_LINE_SIZE + 1];

    if (atm->open_session == 0) {
        printf("No user logged in\n");
        return;
    }

    memset(send_msg, 0, MAX_LINE_SIZE);
    memset(recv_msg, 0, MAX_LINE_SIZE);
    strcat(send_msg, "user-logout ");
    strcat(send_msg, atm->username);

    atm_encrypt(atm, send_msg, strlen(send_msg));
    atm_send(atm, send_msg, MAX_LINE_SIZE);
    atm_recv(atm, recv_msg, MAX_LINE_SIZE);
    atm_decrypt(atm, recv_msg, strlen(recv_msg));

    atm->open_session = 0;
    printf("User logged out\n");
}

void atm_process_command(ATM *atm, char *command)
{
    // TODO: Implement the ATM's side of the ATM-bank protocol

	char cmd[20];
	char arg[20];
	sscanf(command, "%s%s", cmd, arg);
   
	if (strcmp(cmd, "begin-session") == 0) {
		atm_begin_session(arg, atm);
	} else if (strcmp(cmd, "withdraw") == 0) {
		atm_withdraw(arg, atm);
	}	else if (strcmp(cmd, "balance") == 0) {
		atm_balance(atm);
	} else if (strcmp(cmd, "end-session") == 0) {
		atm_end_session(atm);
	} else {
		printf("Invalid command\n");
	}	
}

