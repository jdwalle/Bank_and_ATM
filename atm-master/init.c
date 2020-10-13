#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <stdbool.h>


void generateSeed(char seed[], int seedLength) 
{ 
    int i; 
    for (i = 0; i < seedLength; i++) { 
        int num = (rand() % 10); 
        if (num == 1) {
        	seed[i] = '1';
        } else if (num == 2) {
        	seed[i] = '2';
        } else if (num == 3) {
        	seed[i] = '3';
        } else if (num == 4) {
        	seed[i] = '4';
        } else if (num == 5) {
        	seed[i] = '5';
        } else if (num == 6) {
        	seed[i] = '6';
        } else if (num == 7) {
        	seed[i] = '7';
        } else if (num == 8) {
        	seed[i] = '8';
        } else if (num == 9) {
        	seed[i] = '9';
        } else if (num == 0) {
        	seed[i] = '0';
        }
    }
}

int main(int argc, char *argv[]) {

	if (argc != 2) {
		printf("Usage: init <filename>\n");
		return 62;
	}

	int i;
	int j = 0;
	int indexOf = 0;
	bool hasSlash = false;
	char path[1000] = {'\0'};
	char name[1000] = {'\0'};

	for (i = 0; i < strlen(argv[1]); i++) {
		if (argv[1][i] == '/') {
			indexOf = i;
			hasSlash = true;
		}
	}

	if (!hasSlash) {
		printf("Usage: init <filename>\n");
		return 62;
	}

	for (i = 0; i < indexOf; i++) {
		path[i] = argv[1][i];
	}

	for (i = indexOf + 1; i < strlen(argv[1]); i++) {
		name[j] = argv[1][i];
		j++;
	}

	DIR* dir = opendir(path);
	if (dir) {
		closedir(dir);
	} else {
		printf("Error creating initialization files\n");
		return 64;
	}

	char atm_str[1000] = {'\0'};
	char bank_str[1000] = {'\0'};
	strncpy(atm_str, name, strlen(name));
	strncpy(bank_str, name, strlen(name));
	strcat(atm_str, ".atm");
	strcat(bank_str, ".bank");

	char atm_file_str[1000] = {'\0'};
	char bank_file_str[1000] = {'\0'};
	strncpy(atm_file_str, path, strlen(path));
	strncpy(bank_file_str, path, strlen(path));
	strcat(atm_file_str, "/");
	strcat(bank_file_str, "/");
	strcat(atm_file_str, atm_str);
	strcat(bank_file_str, bank_str);
	
	if (access(atm_file_str, F_OK) != -1) {
		printf("Error: one of the files already exists\n");
		return 63;
	}

	if (access(bank_file_str, F_OK) != -1) {
		printf("Error: one of the files already exists\n");
		return 63;
	}

	FILE *atm_file = fopen(atm_file_str, "w+");
	FILE *bank_file = fopen(bank_file_str, "w+");
	if (atm_file == NULL || bank_file == NULL) {
		printf("Error creating initialization files\n");
		return 64;
	}
	
	srand(time(0));
	int seedLength = 10;
	char seed[11] = "";

	
	generateSeed(seed, seedLength);
	fwrite(seed, 1, sizeof(seed), atm_file);
	fwrite(seed, 1, sizeof(seed), bank_file);

	
	fclose(atm_file);
	fclose(bank_file);
	
	printf("Successfully initialized bank state\n");
	return 0;
}
