#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#include "file_system_call.h"
#include "system_call_helper.h"
#include "user_helper.h"
#include "commands.h"

void move_file(char *src, char *dest) {
    f_move(src, dest);
}

void copy_file(char *src, char *dest) {
    int src_fd = f_open(src, F_READ);
    if (src_fd == -1) {
        p_perror("f_open");
        return;
    }
    int dest_fd = f_open(dest, F_WRITE);
    if (dest_fd == -1) {
        p_perror("f_open");
        return;
    }
    char buf[NREAD];

    int nread = f_read(src_fd, NREAD, buf);
    if (nread == -1) {
        f_close(src_fd);
        f_close(dest_fd);
        p_perror("f_read");
        return;
    }
    while (nread > 0)  {
        if (f_write(dest_fd, buf, nread) == -1) {
            perror("f_write");
            f_close(src_fd);
            f_close(dest_fd);
            return;
        }
        nread = f_read(src_fd, NREAD, buf);
        if (nread == -1) {
            f_close(src_fd);
            f_close(dest_fd);
            perror("f_read");
            return;
        }
    }
    f_close(src_fd);
    f_close(dest_fd);
}

void remove_file(char *files[]) {
    int index = 1;
    while (files[index]) {
        if (f_unlink(files[index]) < 0){
            p_perror("f_unlink");
        }
        index++;
    }
}

void chmod_file(char *mode, char *file) {
    f_chmod(mode, file);
}

void cat(char *stdin_file, char *stdout_file, int is_file_append) {
    int src_fd = STANDARD_IN;
    int dest_fd = STANDARD_OUT;
    if (stdin_file != NULL) {
        src_fd = f_open(stdin_file, F_READ);
        if (src_fd == -1) {
            p_perror("f_open");
            return;
        }
    }
    if (stdout_file != NULL) {
        if (is_file_append) {
            dest_fd = f_open(stdout_file, F_APPEND);
        } else {
            dest_fd = f_open(stdout_file, F_WRITE);
        }
        if (dest_fd == -1) {
            p_perror("f_open");
            if (stdin_file != NULL) {
                f_close(src_fd);
            }
            return;
        }
    }

    char buf[NREAD];

    int nread = f_read(src_fd, NREAD, buf);
    if (nread == -1) {
        p_perror("f_read");
        if (stdin_file != NULL) {
            f_close(src_fd);
        }
        if (stdout_file != NULL) {
            f_close(dest_fd);
        }
        return;
    }
    while (nread > 0)  {
        if (f_write(dest_fd, buf, nread) == -1) {
            p_perror("f_write");
            if (stdin_file != NULL) {
                f_close(src_fd);
            }
            if (stdout_file != NULL) {
                f_close(dest_fd);
            }
            return;
        }
        nread = f_read(src_fd, NREAD, buf);
        if (nread == -1) {
            p_perror("f_read");
            if (stdin_file != NULL) {
                f_close(src_fd);
            }
            if (stdout_file != NULL) {
                f_close(dest_fd);
            }
            return;
        }
    }
    if (stdin_file != NULL) {
        f_close(src_fd);
    }
    if (stdout_file != NULL) {
        f_close(dest_fd);
    }
}

void echo(char *cmd[], char *stdout_file, int is_file_append) {
    int dest_fd = STANDARD_OUT;
    if (stdout_file != NULL) {
        if (is_file_append) {
            dest_fd = f_open(stdout_file, F_APPEND);
        } else {
            dest_fd = f_open(stdout_file, F_WRITE);
        }
        if (dest_fd == -1) {
            return;
        }
    }
    int idx = 1;
    while (cmd[idx]) {
        f_write(dest_fd, cmd[idx], strlen(cmd[idx]));
        f_close(dest_fd);
        if (stdout_file != NULL){
            dest_fd = f_open(stdout_file, F_APPEND);
        }
        idx++;
        if (cmd[idx]) {
            f_write(dest_fd, " ", 1);
        } else {
            f_write(dest_fd, "\n", 1);
        }
    }
    if (stdout_file != NULL) {
        f_close(dest_fd);
    }
}