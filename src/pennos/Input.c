#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Input.h"
#include "Utility.h"
#include "file_system_call.h"
#include "system_call_helper.h"

// Check input type and respond.
ssize_t readInput(char* input) {
    ssize_t numBytes = f_read(STANDARD_IN, BUFFER_SIZE, input);
    // checkError(numBytes, "read");

    // Ctrl-D pressed and command is empty.
    // Return -1 to exit.
    if (numBytes == 0) {
        return -1;
    }

    // Ctrl-D pressed but command is not empty.
    // Return 0 to start reprompt.
    if (input[numBytes-1] != '\n') {
        numBytes = f_write(STANDARD_OUT, "\n", sizeof(char));
        // checkError(numBytes, "write");
        return 0;
    }

    // End command with null terminator.
    input[numBytes] = '\0';
    return numBytes;
}

// Iterate command and count argument number.
int countArgument(char* input, ssize_t numBytes)
{
    if (numBytes <= 0) {
        return numBytes;
    }
    int argCount = 0;
    int i = 0;
    while (i < numBytes) {
        // Skip spaces.
        while (i < numBytes && isSpace(input[i])) {
            ++i;
        }
        if (i == numBytes) {
            break;
        }

        // New argument found.
        ++argCount;

        // Jump to the end of argument.
        while (i < numBytes && !isSpace(input[i])) {
            ++i;
        }
    }
    return argCount;
}

int isSpace(char c) {
    if (c == ' ' || c == '\t' || c == '\n') {
        return 1;
    } else {
        return 0;
    }
}

// Iteratively find tokens separated by spaces.
void parseInput(char* input, int argCount, char** argValue) {
    int argIndex = 0;
    char* pch = strtok(input, " \t\n");
    while (pch != NULL) {
        argValue[argIndex++] = pch;
        pch = strtok(NULL, " \t\n");
    }
    // End array with null terminator.
    argValue[argIndex] = NULL;
}

int isBuiltIn(char* cmd) {
    if (strcmp(cmd, "bg") == 0) {
        return BG_FLAG;
    }
    if (strcmp(cmd, "fg") == 0) {
        return FG_FLAG;
    }
    if (strcmp(cmd, "jobs") == 0) {
        return JOBS_FLAG;
    }
    if (strcmp(cmd, "logout") == 0) {
        return LOGOUT_FLAG;
    }
    if (strcmp(cmd, "nice") == 0) {
        return NICE_FLAG;
    }
    if (strcmp(cmd, "nice_pid") == 0) {
        return NICE_PID_FLAG;
    }
    if (strcmp(cmd, "man") == 0) {
        return MAN_FLAG;
    }
    if (strcmp(cmd, "hang") == 0) {
        return HANG_FLAG;
    }
    if (strcmp(cmd, "nohang") == 0) {
        return NOHANG_FLAG;
    }
    if (strcmp(cmd, "recur") == 0) {
        return RECUR_FLAG;
    }
    if (f_execute(cmd) != NULL) {
        
        return  EXECUTE_FILE_FLAG;
    }
    if (strcmp(cmd, "man") == 0) {
        return MAN_FLAG;
    }
    
    return 0;
}
