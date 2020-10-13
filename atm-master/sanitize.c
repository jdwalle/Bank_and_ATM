#include <stdlib.h> 
#include <string.h>
#include <regex.h>
#include <stdbool.h>

bool sanitizeFilename (char * filename) {

    // Iterate through each char in the username string
    int i;
    for (i = 0; i < strlen(filename); i++) {

        // Check that the current char is not invalid
        if (filename[i] == '!') {

            return false;
        } else if (filename[i] == '@') {

            return false;
        } else if (filename[i] == '#') {

            return false;
        } else if (filename[i] == '$') {

            return false;
        } else if (filename[i] == '%') {

            return false;
        } else if (filename[i] == '^') {

            return false;
        } else if (filename[i] == '&') {

            return false;
        } else if (filename[i] == '*') {

            return false;
        } else if (filename[i] == '(') {

            return false;
        } else if (filename[i] == ')') {

            return false;
        } else if (filename[i] == '@') {

            return false;
        } else if (filename[i] == '\'') {

            return false;
        } else if (filename[i] == '\"') {

            return false;
        } else if (filename[i] == '>') {

            return false;
        } else if (filename[i] == '<') {

            return false;
        } else if (filename[i] == '.') {

            return false;
        } else if (filename[i] == '`') {

            return false;
        } else if (filename[i] == '|') {

            return false;
        } else if (filename[i] == '\\') {

            return false;
        }
    }

    // Valid filename
    return true;
}


// Verifies that the pin is well formatted
bool sanitizePin (char *pin) {

    // Check that the pin has a valid length
    if (strlen(pin) == 4) {

        // Iterate through each char in the pin string
        int i;
        for (i = 0; i < 4; i++) {

            // Check that the current char is a valid num
            if (((int) pin[i]) > 57 || ((int) pin[i]) < 48) {

                return false;
            }
        }

        // Valid pin
        return true;

    } else {

        // Invalid length
        return false;
    }
}

// Verifies that the username is well formatted
bool sanitizeUser (char *username) {

    // Check that the username has a valid length
    if (strlen(username) < 251) {

        // Iterate through each char in the username string
        int i;
        for (i = 0; i < strlen(username); i++) {

            // Check that the current char is a valid num
            if (!((((int) username[i]) < 91 && ((int) username[i]) > 64) || (((int) username[i]) < 123 && ((int) username[i]) > 96))) {

                return false;
            }
        }

        // Valid username
        return true;

    } else {

        // Invalid length
        return false;
    }
}

// Verifies that the amount is well formatted
bool sanitizeAmount (char *amount) {

    // Check that the amount has a valid length
    if (strlen(amount) < 10) {

        // Iterate through each char in the amount string
        int i;
        for (i = 0; i < strlen(amount); i++) {

            // Check that the current char is a valid num
            if (((int) amount[i]) > 57 || ((int) amount[i]) < 48) {

                return false;
            }
        }

        // Valid amount
        return true;
    // Max length of an int
    } else if (strlen(amount) == 10) {

        // Stores the max int (as a string) in c
        char * max_int = "2147483647";

        // Determine if the amount is larger than the max
        if (strcmp(amount, max_int) > 0) {
            return false;
        }

        // Iterate through each char in the amount string
        int i;
        for (i = 0; i < strlen(amount); i++) {

            // Check that the current char is a valid num
            if (((int) amount[i]) > 57 || ((int) amount[i]) < 48) {

                return false;
            }
        }

        // Valid amount
        return true;
    } else {

        // Invalid length
        return false;
    }
}

bool numLength (char *amount) {
    if (strlen(amount) > 10) { //Checking for too long number
        return false; //Number is too long
    }
    return true; //Number is good
}
