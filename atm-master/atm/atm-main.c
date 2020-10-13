/* 
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

static const char prompt[] = "ATM: ";

int main(int argc, char *argv[])
{

    FILE *atm_init_file;

    if (argc != 2) {
        printf("Error opening atm initialization file\n");
        return 64;
    }

    atm_init_file = fopen(argv[1], "r");
    if (atm_init_file == NULL) {
        printf("Error opening atm initialization file\n");
        return 64;
    }

    /* Seek to the beginning of the file */
    fseek(atm_init_file, 0, SEEK_SET);

    /* Read the seed */
    char seed[11];
    fread(seed, 10, 1, atm_init_file);
    seed[10] = '\0';

    /* Convert the seed to an int and intialize the random generator */
    int init_seed = atoi(seed);
    generate_rand(init_seed);

    char user_input[1000];

    ATM *atm = atm_create();

    printf("%s", prompt);
    fflush(stdout);
    char currUsr[251];
    currUsr[0] = '\0';
    while (fgets(user_input, 10000,stdin) != NULL)
    {   
        strcpy(currUsr, atm_process_command(atm, user_input, currUsr));
        if(currUsr[0] != '\0'){
            printf("\nATM (%s): ", currUsr);
        } else {
            printf("\nATM: ");
        }
        fflush(stdout);
        bzero((void *) user_input, 1000);
    }

    fclose(atm_init_file);

    return EXIT_SUCCESS;
}

