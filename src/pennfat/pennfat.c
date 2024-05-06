#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>

#include "commands.h"
#include "helper.h"

#define MAX_INPUT 1024
#define MAX_LIST 100

//check command and do following execution
void do_command(char *Args[], int argv){
    if (strcmp(Args[0], "mkfs") == 0){
        if (argv != 4){
            perror("invalid arh=gument");
            return;
        }
        do_mkfs(Args);
    }
    else if (strcmp(Args[0], "mount") == 0){
        if (argv != 2){
            perror("invalid arh=gument");
            return;
        }
        do_mount(Args[1]);
    }
    else if (strcmp(Args[0], "umount") == 0){
        if (argv != 1){
            perror("invalid arh=gument");
            return;
        }
        do_umount();
    }
    else if (strcmp(Args[0], "touch") == 0){
        for (int i = 1; i < argv; i++){
            do_touch(Args[i]);
        }
    }
    else if (strcmp(Args[0], "mv") == 0){
        if (do_mv(Args) == -1){
            fprintf(stderr, "%s doesn't exist\n", Args[1]);
        }
    }
    else if (strcmp(Args[0], "rm") == 0){
        for (int i = 1; i < argv; i++){
            if (do_rm(Args[i]) == -1){
                fprintf(stderr, "%s doesn't exist\n", Args[i]);
            }
        }
    }
    else if (strcmp(Args[0], "cat") == 0){
        if (argv == 3) {
            if (strcmp(Args[1], "-w") == 0){
                char inputBuffer[MAX_SIZE];
                memset(&inputBuffer, 0, MAX_SIZE);
                ssize_t nread = read(STDIN_FILENO, inputBuffer, MAX_SIZE);
                if (nread < 0){
                    perror("read");
                    return;
                }
                do_cat_overwrite(nread, inputBuffer, Args[2]);
                memset(&inputBuffer, 0, MAX_SIZE);
            } else if (strcmp(Args[1], "-a") == 0){
                char inputBuffer[MAX_SIZE];
                memset(&inputBuffer, 0, MAX_SIZE);
                ssize_t nread = read(STDIN_FILENO, inputBuffer, MAX_SIZE);
                if (nread < 0){
                    perror("read");
                    return;
                }
                do_cat_append(nread, inputBuffer, Args[2]);
                memset(&inputBuffer, 0, MAX_SIZE);
            } else {
                char *concat_files[MAX_LIST];
                int i, count = 0;
                for (i = 1; i < argv; i++){
                    concat_files[count] = Args[i];
                    count++;
                }
                concat_files[count] = NULL;
                do_cat(concat_files);
            }
        } else {
            char *concat_files[MAX_LIST];
            char dest[32];
            int overwrite = -1, count = 0, i;
            for (i = 1; i < argv - 1; i++){
                if (strcmp(Args[i], "-w") == 0){
                    overwrite = 0;
                    continue;
                } else if (strcmp(Args[i], "-a") == 0){
                    overwrite = 1;
                    continue;
                }
                concat_files[count] = Args[i];
                count++;
            }
            if (overwrite == -1){
                concat_files[count] = Args[i];
                concat_files[count + 1] = NULL;
                do_cat(concat_files);
            } else if (overwrite == 0) {
                concat_files[count] = NULL;
                strcpy(dest, Args[i]);
                do_concat_overwrite(concat_files, dest);
            } else {
                concat_files[count] = NULL;
                strcpy(dest, Args[i]);
                do_concat_append(concat_files, dest);
            }
        }
    }
    else if (strcmp(Args[0], "ls") == 0){
        do_ls();
    }
    else if (strcmp(Args[0], "cp") == 0){
        if (argv == 4 && (strcmp(Args[2], "-h") == 0)) {
            do_cpto_host(Args[1], Args[3]);
        } else {
            do_cp(Args, argv);
        }
    }
    else if (strcmp(Args[0], "chmod") == 0){
        do_chmod(Args[1], Args[2]);
    } else {
        perror("invalid command");
    }
}

int main(){
    char *Args[MAX_LIST];
    while(1){
        write(STDERR_FILENO, "pennfat# ", 10);
        char *inputBuffer = (char *)malloc(MAX_INPUT);
        int n = read(STDIN_FILENO, inputBuffer, MAX_INPUT);

        if (n == 0 && inputBuffer[0] != '\n'){
            fprintf(stderr, "\n");
            free(inputBuffer);
            exit(0);
        }
        
        trim_command(inputBuffer);
        int index = 0;
        char *arg = strtok(inputBuffer, " ");
        while (arg){
            Args[index] = arg;
            index++;
            arg = strtok(NULL, " ");
        }
        Args[index] = NULL;
        do_command(Args, index);
        free(inputBuffer);

    }
    return 0;
}