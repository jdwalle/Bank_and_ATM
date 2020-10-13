#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <fcntl.h> 

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

    unsigned char key[33] = "";
    generate_key(key);

    unsigned char *iv = (unsigned char *)"4675742963217414";
    unsigned char buf[1000];
    int length = encrypt(data, strlen(data), key, iv, buf);
    buf[length] = '\0';

    // Returns the number of bytes sent; negative on error
    return sendto(atm->sockfd, buf, length, 0,
                  (struct sockaddr*) &atm->rtr_addr, sizeof(atm->rtr_addr));
}

ssize_t atm_recv(ATM *atm, char *data, size_t max_data_len)
{

    // Returns the number of bytes received; negative on error
    return recvfrom(atm->sockfd, data, max_data_len, 0, NULL, NULL);
}

char* atm_process_command(ATM *atm, char *command, char* currUsr)
{
    //max int 2147483647
    //char* token;

    int test = strlen(command);

    char* cmd = strtok_r(command, " ", &command);
    char* arg = strtok_r(command, " ", &command);
    char* badCmd = strtok_r(command, " ", &command);

    int zero = 0;
    int one = 0;
    int two = 0;
    int total = 0;
    int wordCount = 0;
    bool badSpaces = false;

    if (cmd) {
        zero = strlen(cmd);
        wordCount++;  
    }

    if (arg) {
        one = strlen(arg);
        wordCount++;
    }

    if (badCmd) {
        two = strlen(badCmd);
        wordCount++;  
    }

    total = zero + one + two + (wordCount - 1);


    if (total != test) {
        badSpaces = true;
    }

    if(arg && arg[strlen(arg)-1] == '\n'){
        arg[strlen(arg) -1] = '\0';
    }
     if(cmd && cmd[strlen(cmd)-1] == '\n'){
        cmd[strlen(cmd) -1] = '\0';
    }

    if(strcmp(cmd, "begin-session") == 0){
        if(currUsr[0] != '\0'){
            printf("A user is already logged in\n");
        }   else if (badCmd || (arg && (strlen(arg) == 0))){
            printf("Usage: begin-session <user-name>\n");
        } else if(!arg || badSpaces){
            printf("Usage: begin-session <user-name>\n");
        } else {
            char cards[256];
            strcpy(cards, "./");
            strcat(cards, arg);
            strcat(cards, ".card");
            // printf("%s", cards);
            if( access(cards , F_OK ) != -1 ) {
                FILE *cardReader = fopen(cards, "r");
                if(cardReader==NULL){
                    printf("Unable to access %s's card\n", arg);
                } else {
                    printf("PIN? ");
                    char pin[250];
                    if(fgets(pin, 250,stdin) != NULL){
                        for(int idx = 0; idx < 250; idx++){
                            if(pin[idx] == '\n'){
                                pin[idx] = '\0';
                            }
                        }
                        pin[250] = '\0';
                        if (strlen(pin) != 4) {
                            printf("Not authorized\n");
                            currUsr[0] = '\0';
                            return currUsr;
                        }
                        char sendCmd[400] = "valid-pin ";
                        strcat(sendCmd, arg);
                        strcat(sendCmd, " ");
                        strcat(sendCmd, pin);
                        //printf("%sa", sendCmd);
                        int byt;
                        char recvline[10000];
                        atm_send(atm, sendCmd, strlen(sendCmd));
                        byt = atm_recv(atm,recvline,10000);
                        recvline[byt]=0;

                        unsigned char key[33] = { '\0' };
                        bzero(key, 33);
                        unsigned char *iv = (unsigned char *)"0000000000000000";
                        unsigned char plaintext[300];

                        generate_key(key);
                          
                        int length = decrypt(recvline, byt, key, iv, plaintext);
                        plaintext[length] = '\0';

                        if(strcmp(plaintext, "1") == 0) {
                            printf("Authorized\n");
                            return arg;
                        } else {
                            printf("Not authorized\n");
                            currUsr[0] = '\0';
                            return currUsr;
                        }
                    } else {
                        printf("Not authorized\n");
                        return currUsr;
                    }
                }
            } else {
                printf("No such user\n");
                currUsr[0] = '\0';
                return currUsr;
            }
        }
    } else if(strcmp(cmd, "balance") == 0) {
        if(currUsr[0] == '\0' ){//no user logged
            printf("No user logged in\n");
            return currUsr;
        } else if(arg || badSpaces){ //invalid input
            printf("Usage: balance\n");
            return currUsr;
        }else{
            char sendCmd[400] = "balance ";
            strcat(sendCmd, currUsr);
            int byt;
            char recvline[10000];
            atm_send(atm, sendCmd, strlen(sendCmd));
            byt = atm_recv(atm,recvline,10000);
            recvline[byt]=0;

            unsigned char key[33] = { '\0' };
            bzero(key, 33);
            unsigned char *iv = (unsigned char *)"0000000000000000";
            unsigned char plaintext[300];

            generate_key(key);
              
            int length = decrypt(recvline, byt, key, iv, plaintext);
            plaintext[length] = '\0';

            printf("$%s\n", plaintext);
            return currUsr;
        }
    } else if(strcmp(cmd, "withdraw") == 0){
      
        if(currUsr[0] == '\0'){
            printf("No user logged in\n");
        } else if (badCmd || (arg && (strlen(arg) == 0))){
            printf("Usage: withdraw <amt>\n");
        } else if (!arg || badSpaces){
            printf("Usage: withdraw <amt>\n");
        } else {
            
            char sendCmd[400];
            bzero(sendCmd, 400);
            strcat(sendCmd, "withdraw ");
            strcat(sendCmd, currUsr);
            strcat(sendCmd, " ");
            strcat(sendCmd, arg);

            int byt2;
            char recvline2[10000];

            atm_send(atm, sendCmd, strlen(sendCmd));
            byt2 = atm_recv(atm,recvline2,10000);
            recvline2[byt2]=0;

            unsigned char key[33] = { '\0' };
            bzero(key, 33);
            unsigned char *iv = (unsigned char *)"0000000000000000";
            unsigned char plaintext[300];

            generate_key(key);
              
            int length = decrypt(recvline2, byt2, key, iv, plaintext);
            plaintext[length] = '\0';

            if(strcmp(plaintext, "1") != 0) {
                printf("Insufficient funds\n");
            } else {
                printf("$%s dispensed\n", arg);
            }

            return currUsr;
        }
    } else if(strcmp(cmd, "end-session") == 0){
        if(currUsr[0] == '\0'){ //no user logged in
            printf("No user logged in\n");
            return currUsr;
        } else {
            currUsr[0] = '\0';
            printf("User logged out\n");
            return currUsr;
        }   
    } else {
        printf("Invalid command\n");
    }
    
    return currUsr;
}
