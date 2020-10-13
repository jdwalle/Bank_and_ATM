/* 
 * The main program for the Bank.
 *
 * You are free to change this as necessary.
 */

#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "bank.h"
#include "ports.h"

static const char prompt[] = "BANK: ";

int main(int argc, char *argv[])
{

  FILE *bank_init_file;

  if (argc != 2) {
    printf("Error opening bank initialization file\n");
    return 64;
  }

  bank_init_file = fopen(argv[1], "r");

  if (bank_init_file == NULL) {
    printf("Error opening bank initialization file\n");
    return 64;
  } 

  /* Seek to the beginning of the file */
  fseek(bank_init_file, 0, SEEK_SET);

  /* Read the seed */
  char seed[11];
  fread(seed, 10, 1, bank_init_file);
  seed[10] = '\0';

  /* Convert the seed to an int and intialize the random generator */
  int init_seed = atoi(seed);
  generate_rand(init_seed);

  /* Generate th iv to print */
  //char key[33] = {'\0'};
  //generate_key(key);
  //printf("Bank Key:%s\n", key);

  int n;
  char safeBuffer[1001];
  char trash[10000];
  char sendline[1000];
  char recvline[1000];

  Bank *bank = bank_create();

  printf("%s", prompt);
  fflush(stdout);

  while(1)
  {
     bzero((void *) sendline, 1000);
     bzero((void *) recvline, 1000);
     fd_set fds;
     FD_ZERO(&fds);
     FD_SET(0, &fds);
     FD_SET(bank->sockfd, &fds);
     select(bank->sockfd+1, &fds, NULL, NULL, NULL);

     if(FD_ISSET(0, &fds))
     {
       fgets(safeBuffer, 1000, stdin);
       safeBuffer[1001] = '\0';
       bool trashVal = false;
       if (strlen(safeBuffer) > 300) {
        trashVal = true;
        while (trashVal) {
          // Do nothing, clear it out
          fgets(trash, 10000, stdin);
          char nully[10000] = { '\0' };
          if (strcmp(trash, nully)) {
            trashVal = false;
          }
        }
       } 
      strncpy(sendline, safeBuffer, 1000);
      bank_process_local_command(bank, sendline, strlen(sendline));
      printf("\n%s", prompt);
      fflush(stdout);
        
     }
     else if(FD_ISSET(bank->sockfd, &fds))
     {
        n = bank_recv(bank, recvline, 300);

        //char sizedBuffer[301] = { '\0' };
        //bzero(sizedBuffer, 301);

        //strncpy(sizedBuffer, recvline, sizeof(sizedBuffer) - sizeof(char));

        unsigned char key[33] = "";
        unsigned char *iv = (unsigned char *) "4675742963217414";
        unsigned char plaintext[300];
    
        generate_key(key);
          
        int length = decrypt(recvline, n, key, iv, plaintext);
        plaintext[length] = '\0';

        bzero(key, 33);
        key[0] = '\0';
        fflush(stdout);

        bank_process_remote_command(bank, plaintext, n);
     }
   }

   fclose(bank_init_file);

   return EXIT_SUCCESS;
}
