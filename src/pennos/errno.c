#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errno.h"

int error_code = 0;

void p_perror(char *message){
    switch(error_code){
        case SUCCESS:
            fprintf(stderr, "%s: %s", message, "Success");
            error_code = 0;
            break;
        case MULTIPLE_WRITE:
            fprintf(stderr, "%s: %s", message, "Multiple f_write");
            error_code = 0;
            break;
        case FAT_FULL:
            fprintf(stderr, "%s: %s", message, "FAT is full");
            error_code = 0;
            break;
        case PERMISSION:
            fprintf(stderr, "%s: %s", message, "Permission denied");
            error_code = 0;
            break;
        case INVALID_COMMAND:
            fprintf(stderr, "%s: %s", message, "Invalid command");    
            error_code = 0;
            break;
        case INVALID_FD:
            fprintf(stderr, "%s: %s", message, "file does not exist"); 
            error_code = 0;
            break;
        case PID:
            fprintf(stderr, "%s: %s", message, "PID not found"); 
            error_code = 0;
            break;
        case INVALID_MOD:
            fprintf(stderr, "%s: %s", message, "invalid mod"); 
            error_code = 0;
            break;
        case SIG_ERROR:
            fprintf(stderr, "%s: %s", message, "unable to catch signal"); 
            error_code = 0;
            break;
        default:
            fprintf(stderr, "%s: %s", message, "failure"); 
            error_code = 0;
            break;
    }
}