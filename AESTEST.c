#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <openssl/aes.h>

/* AES key for Encryption and Decryption */
/*
void atm_encrypt1(char *str, int len) {
char aes_key[]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
	int i;
	for (i = 0; i < len; i++) {
		str[i] ^= aes_key[i];		
	}
}

void atm_decrypt1(char *str, int len) {
char aes_key[]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
	int i;
	for (i = 0; i < len; i++) {
		str[i] ^= aes_key[i];		
	}
}

 
int main() {

	char shit[] = "AAAAAAA";
	atm_encrpt1(shit, 7);
	printf("%s\n", shit);
	atm_decrpt1(shit, 7);
	printf("%s\n", shit);
	return 0;
}*/

void enc(char *str, int len) {
	unsigned char key[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC};
	int i;
	for (i = 0; i < len; i++) {
		str[i] ^= key[i];		
	}
}

void dec(char *str, int len) {
	unsigned char key[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC};
	int i;	
	for (i = 0; i < len; i++) {
		str[i] ^= key[i];		
	}
}
	
int main() {
	char shit[] = "AAAAAAA";
	enc(shit, 7);
	dec(shit, 7);
	printf ("%s", shit);
	return 0;	
}


