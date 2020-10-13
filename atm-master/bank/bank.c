#include "bank.h"
#include "ports.h"
#include "hash_table.h"
#include "sanitize.c"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <stdbool.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <fcntl.h> 

// Change this to test user overflow
static const int userCapacity = 200;

void generate_rand(seed){
  srand(seed);
}

void generate_key(unsigned char *key){
    
  int num = 0;
  for(int i = 0; i < 32; i++){
    num = rand() % 10;
    if (num == 1) {
      key[i] = '1';
    } else if (num == 2) {
      key[i] = '2';
    } else if (num == 3) {
      key[i] = '3';
    } else if (num == 4) {
      key[i] = '4';
    } else if (num == 5) {
      key[i] = '5';
    } else if (num == 6) {
      key[i] = '6';
    } else if (num == 7) {
      key[i] = '7';
    } else if (num == 8) {
      key[i] = '8';
    } else if (num == 9) {
      key[i] = '9';
    } else if (num == 0) {
      key[i] = '0';
    }
  }
  key[33] = '\0';
}

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}


Bank* bank_create()
{
    Bank *bank = (Bank*) malloc(sizeof(Bank));
    if(bank == NULL)
    {
        perror("Could not allocate Bank");
        exit(1);
    }

    bzero(&bank->acc_table, sizeof(bank->acc_table));
    bank->acc_table = hash_table_create(userCapacity); // Creating dynamically allocated hash table with 200 bins

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

    return bank;
}

void bank_free(Bank *bank)
{
    if(bank != NULL)
    {
        close(bank->sockfd);
        hash_table_free(bank->acc_table);
        free(bank);
    }
}

ssize_t bank_send(Bank *bank, char *data, size_t data_len)
{
  unsigned char key[33] = { '\0' };
  generate_key(key);
  unsigned char *iv = (unsigned char *)"0000000000000000";
  unsigned char buf[1000];

  int length = encrypt(data, data_len, key, iv, buf);

  // Returns the number of bytes sent; negative on error
  return sendto(bank->sockfd, buf, length, 0,
                (struct sockaddr*) &bank->rtr_addr, sizeof(bank->rtr_addr));
}

ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len)
{

  // Returns the number of bytes received; negative on error
  return recvfrom(bank->sockfd, data, max_data_len, 0, NULL, NULL);
}

// Helper function that determines if an amount was not valid
// because it was too large and not because of other reasons
bool tooLarge (char * amount) {

  // Iterate through each char in the amount string
  int i;
  for (i = 0; i < strlen(amount); i++) {

      // Check that the current char is a valid num
      if (((int) amount[i]) > 57 || ((int) amount[i]) < 48) {

          return false;
      }
  }

  // Reason the amount was rejected was because of its size
  return true;
}

void bank_process_local_command(Bank *bank, char *command, size_t len)
{
  /*
   * Create a char pointer array to store command tokens,
   * read in the tokens to the corresponding pointers
   */
  char *buffer[2000] = {'\0'};
  strncpy(buffer, command, 2000);
  char * arguments[5];
  arguments[0] = strtok_r(command, " ", &command);
  arguments[1] = strtok_r(command, " ", &command);
  arguments[2] = strtok_r(command, " ", &command);
  arguments[3] = strtok_r(command, " ", &command);
  arguments[4] = strtok_r(command, " ", &command);

  int zero;
  int one;
  int two;
  int three;
  int overflow = 0;
  // Keep track of the arguments provided
  int wordsResult = 5;

  // Make sure too many arguments werent provided
  if (!arguments[4]) {
    wordsResult--;
  } else {
    // Too many arguments provided
    wordsResult = -1;
  }

  // Make sure too many arguments werent provided (for balance and deposit)
  if (!arguments[3]) {
    wordsResult--;
  } else {
    three = strlen(arguments[3]);
    // Fix a formatting issue that may occur 
    if (arguments[3][strlen(arguments[3]) - 1] == '\n' || arguments[3][strlen(arguments[3]) - 1] == ' ') {
      arguments[3][strlen(arguments[3]) - 1] = '\0';
      three = strlen(arguments[3]);
    }
    overflow = -1;
  }

  // Make sure too many arguments werent provided (for balance and deposit)
  if (!arguments[2]) {
    wordsResult--;
  } else {
    two = strlen(arguments[2]);
    // Fix a formatting issue that may occur
    if (arguments[2][strlen(arguments[2]) - 1] == '\n' || arguments[2][strlen(arguments[2]) - 1] == ' ') {
      arguments[2][strlen(arguments[2]) - 1] = '\0';
      two--;
    }
  }

  // Determine if too few arguments were provided
  if (!arguments[1]) {
    wordsResult--;
  } else {
    one = strlen(arguments[1]);
    // Fix a formatting issue that may occur
    if (arguments[1][strlen(arguments[1]) - 1] == '\n' || arguments[1][strlen(arguments[1]) - 1] == ' ') {
      arguments[1][strlen(arguments[1]) - 1] = '\0';
      one--;
    }
  }

  // Impossible
  if (!arguments[0]) {
    wordsResult--;
  } else {
    zero = strlen(arguments[0]);
    // Fix a formatting issue that may occur
    if (arguments[0][strlen(arguments[0]) - 1] == '\n' || arguments[0][strlen(arguments[0]) - 1] == ' ') {
      arguments[0][strlen(arguments[0]) - 1] = '\0';
      zero--;
    }
  }

  int total = zero + one + two + three + wordsResult;
  // Determine if too few arguments were provided

  bool bad_spaces = false;

  if (total != len) {
    bad_spaces = true;
  }

  if (wordsResult == 0) {
    printf("Invalid command\n");
  } else {

    // Determine if create-user was the specified command
    if (strcmp(arguments[0], "create-user") == 0) {

      if(hash_table_size(bank->acc_table) == userCapacity){
        printf("Error: user capacity reached\n");
      } else {

        // Make sure the # of arguments and formatting are valid
        if (wordsResult != 4 || !(sanitizeUser(arguments[1])) || !(sanitizePin(arguments[2])) || !(sanitizeAmount(arguments[3])) || bad_spaces) {

          // Formatting or # of arguments was invalid
          printf("Usage:  create-user <user-name> <pin> <balance>\n");
        } else {

          // Create a buffer to store the info for user's card lookup
          char file[1000];
          bzero(file, 1000);
          strcpy(file, arguments[1]);
          strcat(file, ".card");

          // Make sure the user does not have a card and that the hash table doesnt contain them
          if(access(file, F_OK) != -1 || hash_table_find(bank->acc_table, arguments[1]) != NULL) {

            // An error occurred in seeing if they exist
            printf("Error: user %s already exists\n", arguments[1]);
          } else {

            // Create the users card
            FILE *card_file = fopen(file, "w+");
            if(card_file == NULL){

              // Could not create the user's card
              printf("Error creating card file for user %s\n", arguments[1]);
            } else {

              // User's card was created successfully
              fclose(card_file);

              /*
               * Creating a new entry for the users account in the hash table,
               * the pin and initial balance are added to this entry
               */
              char value[1000];
              bzero(value, 1000);
              strcpy(value, arguments[2]);
              strcat(value, " ");
              strcat(value, arguments[3]);

              // A deep copy of the username is created
              char key[251];
              bzero(key, 251);
              strncpy(key, arguments[1], strlen(arguments[1]));

              // The hash table is updated to contain this user
              hash_table_add(bank->acc_table, key, value);

              // A successful indication of creation is returned to the CLI
              printf("Created user %s\n", arguments[1]);
            }
          }
        }
      }
    // Checking to see if balance is the specified command
    } else if (strcmp(arguments[0], "balance") == 0) {

      // Checking the number of arguments and their formatting is appropriate
      if (wordsResult != 2 || !(sanitizeUser(arguments[1])) || bad_spaces || (strlen(arguments[1]) == 0)) {

        // Return indication of an invalid number of arguments or formatting
        printf("Usage:  balance <user-name>\n");
      } else {

        // Determine if the user exists on the hash table
        if(hash_table_find(bank->acc_table, arguments[1]) == NULL) {

          // User was not found and an indication was returned
          printf("No such user\n");
        } else {
          
          // THe user's account is scanned from the hash table
          char * userContents = (char *) hash_table_find(bank->acc_table, arguments[1]);
          char pin[5];
          char balance[11];
          sscanf(userContents, "%s %s", pin, balance);

          // The balance found in the hash is returned to the CLI
          printf("$%s\n", balance);
        }
      }
    // Checking to see if deposit is the specified command
    } else if (strcmp(arguments[0], "deposit") == 0) {

      // Determining if the correct number of arguments and formatting for them applies
      if (wordsResult != 3 || !(sanitizeUser(arguments[1])) || (arguments[2] != NULL && !(sanitizeAmount(arguments[2]))) || bad_spaces) {

        if (bad_spaces) {
          printf("Usage:  deposit <user-name> <amount>\n");

        /*
         * The input was rejected,
         * check to see if amount was too large
         */
        } else if (arguments[2] != NULL && tooLarge(arguments[2]) &&  overflow != -1 && wordsResult != 3) {

          // The inputted amount was too large for this program
          printf("Too rich for this program\n");
        } else if (arguments[2] != NULL && !numLength(arguments[2])) {
          printf("Too rich for this program\n");
        } else {

          // The formatting was incorrect or too many arguments were provided
          printf("Usage:  deposit <user-name> <amount>\n");
        }
      } else {

        // Check to see if the user is found on the hash table
        if (hash_table_find(bank->acc_table, arguments[1]) == NULL) {
          printf("No such user\n");
        } else {
          
          // Scan the pin and balance in the account
          char * userContents = (char *) hash_table_find(bank->acc_table, arguments[1]);
          char pin[5];
          char balance[11];
          sscanf(userContents, "%s %s", pin, balance);

          // Convert the balance and amount to ints
          int bal = atoi(balance);
          int amount = atoi(arguments[2]);

          // Add the amount to the balance
          int attempt = bal + amount;

          // Determine if an int overflow would occur
          if (attempt < 0) {

            // Adding the amount to the balance would overflow the int
            printf("Too rich for this program\n");
          } else {

            /*
             * The deposit will not result in an overflow,
             * update the balance / entry to reinsert into the hash table
             */
            bal = attempt;
            char newValue[20];
            char funds[11];
            sprintf(funds, "%d", bal);
            sprintf(newValue, "%s %s", pin, funds);

            // Make a deep copy of the username in a buffer (redundant)
            char username[251];
            strncpy(username, arguments[1], strlen(arguments[1]));
            username[strlen(arguments[1])] = '\0';

            // Update the hash table
            hash_table_del(bank->acc_table, username);
            hash_table_add(bank->acc_table, username, newValue);

            // Return indication of a succesful deposit
            printf("$%d added to %s's account\n", amount, username);
          }
        }
      }
    } else {

      // Return indication of an improper command
      printf("Invalid command\n");
    }
  }
}




void bank_process_remote_command(Bank *bank, char *command, size_t len)
{

  /*
   * Create a char pointer array to store command tokens,
   * read in the tokens to the corresponding pointers
   */
  char * arguments[4];
  arguments[0] = strtok_r(command, " ", &command);
  arguments[1] = strtok_r(command, " ", &command);
  arguments[2] = strtok_r(command, " ", &command);
  arguments[3] = strtok_r(command, " ", &command);

  // keep track of the # of arguments provided
  int wordsResult = 4;

  // Determine if there was an extra argument provided
  if (!arguments[3]) {
    wordsResult--;
  } else {
    wordsResult = -1;
  }

  // Determine if there was an extra argument provided
  if (!arguments[2]) {
    wordsResult--;
  } else {
    // Replace the newline with a terminator
    if(arguments[2][strlen(arguments[2]) - 1] == '\n'){
      arguments[2][strlen(arguments[2]) - 1] = '\0';
    }
  }

  // Cehck that this argument exists (must for any valid command)
  if (arguments[1]){
    // Replace the newline with a terminator
    if (arguments[1][strlen(arguments[1]) - 1] == '\n') {
      arguments[1][strlen(arguments[1]) - 1] = '\0';
    }
  } else {
    // Indicate too few arguments were provided
    wordsResult = -1;
  }

  // Check to see if an error in argument formatting exists
  if (wordsResult == -1) {

    // Too many / few arguments
    char requestResult[1000] = "-1";
    bank_send(bank, requestResult, strlen(requestResult));
  } else {

    // Now we check that the first argument corresponds to a valid command
    if (strcmp(arguments[0], "valid-pin") == 0) {

      // Check that "valid-pin" has a correct amount of args
      if (wordsResult == 3) {

        // Valid # of arguments provided, now santize inputs
        if (sanitizeUser(arguments[1]) && sanitizePin(arguments[2])) {

          /*
           * Username was well formatted, now process command
           * by confirming the user exists
           */
          void * userAddress = hash_table_find(bank->acc_table, arguments[1]);
          if (userAddress != NULL) {
            /*
             * Username was found, now scan its pin
             */
            char * userContents = (char *) userAddress;
            char pin[4];
            char balance[10];
            sscanf(userContents, "%s %s", pin, balance);

            /* Determine if the pin was valid */
            if (strcmp(pin, arguments[2]) == 0) {

              /* Pin was valid, indicate successful pin lookup */
              char requestResult[] = "1";
              bank_send(bank, requestResult, strlen(requestResult));
            } else {

              /* Pin was not valid, indicate unsuccessful lookup */
              char requestResult[] = "0";
              bank_send(bank, requestResult, strlen(requestResult));
            }
          } else {

            /*
             * Username was not contained in the hash-table,
             * return an indication of this failure
             */
            char requestResult[] = "-2";
            bank_send(bank, requestResult, strlen(requestResult));
          }
        } else {

          // Arguments not well formatted, return an indication of this
          char requestResult[1000] = "-3";
          bank_send(bank, requestResult, strlen(requestResult));
        }
      } else {

        // Invalid # of arguments for "valid-pin", return an indication of this
        char requestResult[] = "-4";
        bank_send(bank, requestResult, strlen(requestResult));
      }
    } else if (strcmp(arguments[0], "balance") == 0) {

      // Check that "balance" has only 1 argument (+ command)
      if (wordsResult == 2) {

        // Valid # of arguments provided, now sanitize the username
        if (sanitizeUser(arguments[1])) {

          /*
           * Username was well formatted, now process command
           * by confirming the user exists
           */
          void * userAddress = hash_table_find(bank->acc_table, arguments[1]);
          if (userAddress != NULL) {

            /*
             * Username was found, now return its balance
             */
            char * userContents = (char *) userAddress;
            char pin[4];
            char balance[10];
            sscanf(userContents, "%s %s", pin, balance);
            bank_send(bank, balance, strlen(balance));

          } else {

            /*
             * Username was not contained in the hash-table,
             * return an indication of this failure
             */
            char requestResult[] = "-1";
            bank_send(bank, requestResult, strlen(requestResult));

          }
        } else {

          // Username was not a valid username format
          char requestResult[] = "-1";
          bank_send(bank, requestResult, strlen(requestResult));

        }
      } else {

        // Invalid # of arguments for "balance"
        char requestResult[] = "-1";
        bank_send(bank, requestResult, strlen(requestResult));
      }
    } else if (strcmp(arguments[0], "withdraw") == 0) {

      // Check that "withdraw" has correct number of arguments
      if (wordsResult == 3) {

        // Valid # of arguments provided, now sanitize username & amount
        if (sanitizeUser(arguments[1]) && sanitizeAmount(arguments[2])) {

          /*
           * Username & amount were well formatted, now process command
           * by confirming the user exists
           */
          void * userAddress = hash_table_find(bank->acc_table, arguments[1]);
          if (userAddress != NULL) {

            /*
             * Username was found, now scan the pin and balance
             */
            char pin[5];
            char balance[11];
            sscanf(userAddress, "%s %s", pin, balance);
            pin[4] = '\0';

            // Convert the strings to ints
            int bal = atoi(balance);
            int amount = atoi(arguments[2]);

            // Determine if the user has sufficient funds
            if (bal >= amount) {

              // Subtract the amount to withdraw from the amount currently in the account
              int remainingFunds = bal - amount;

              // Create a new value with the updated funds or the hash
              char newValue[20];
              char funds[11];
              sprintf(funds, "%d", remainingFunds);
              sprintf(newValue, "%s %s", pin, funds);

              // Place the username in a buffer for rehashing (redundant)
              char username[251];
              strncpy(username, arguments[1], strlen(arguments[1]));
              username[strlen(arguments[1])] = '\0';

              // Update the hash
              hash_table_del(bank->acc_table, username);
              hash_table_add(bank->acc_table, username, newValue);

              // Return indication of a successful withdraw
              char requestResult[] = "1";
              bank_send(bank, requestResult, strlen(requestResult));
            } else {

              // Insufficient funds
              char requestResult[] = "0";
              bank_send(bank, requestResult, strlen(requestResult));
            }
          } else {

            /*
             * Username was not contained in the hash-table,
             * return an indication of this failure
             */
            char requestResult[] = "-1";
            bank_send(bank, requestResult, strlen(requestResult));
          }
        } else {

          // Username or amount was not valid
          char requestResult[] = "-2";
          bank_send(bank, requestResult, strlen(requestResult));
        }
      } else {

        // Invalid # of arguments for "withdraw"
        char requestResult[] = "-3";
        bank_send(bank, requestResult, strlen(requestResult));
      }
    } else {

      // Invalid command provided
      char requestResult[] = "-9";
      bank_send(bank, requestResult, strlen(requestResult));
    }

  }
}




