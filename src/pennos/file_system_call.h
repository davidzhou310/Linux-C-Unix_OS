#ifndef FILE_SYSTEM_CALL
#define FILE_SYSTEM_CALL

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
#include "system_call_helper.h"
#include "errno.h"

extern int fs_fd;
extern uint16_t *fat;
extern int page_size;
extern int error_code;

typedef struct file_command file_cmd;

struct file_command{
    int argv;
    char *command_line[MAX_LIST];
};

int f_open(const char *fname, int mode);

int f_read(int fd, int n, char *buf);

int f_write(int fd, const char *str, int n);

int f_close(int fd);

int f_unlink(const char *file_name);

int f_lseek(int fd, int offset, int whence);

void f_ls(const char *file_name);

int f_move(char *src, char *dest);

void f_chmod(char *mode, char *file);

int f_mount(char *file_system);

int f_umount();

file_cmd *f_execute(char *file_name);
#endif
