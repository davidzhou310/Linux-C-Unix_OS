#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h> 
#include <sys/types.h>
#include <sys/wait.h>

#include "Utility.h"
#include "parser.h"
#include "file_system_call.h"

// Reprompt when Ctrl-C is pressed.
void repromptHandler(int signum) {
    prompt(NEW_LINE);
}

int prompt(int newLine) {
    char PROMPT[] = "$>";
    if (newLine) {
        ssize_t numBytes = f_write(STANDARD_OUT, "\n", sizeof(char));
    }
    ssize_t numBytes = f_write(STANDARD_OUT, PROMPT, strlen(PROMPT));
    return 0;
}