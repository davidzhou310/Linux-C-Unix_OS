#ifndef SYSTEM_CALL_HELPER
#define SYSTEM_CALL_HELPER

#include  "commands_helper.h"

#define STANDARD_IN 0
#define STANDARD_OUT 1

#define F_WRITE -1
#define F_READ 0
#define F_APPEND 1

#define F_SEEK_SET 0
#define F_SEEK_END -1
#define F_SEEK_CUR 1

typedef struct file_descriptor file_descriptor;

struct file_descriptor {
    char *file_name;
    int perm;
    int descriptor;
    int file_pointer; 
    struct file_descriptor *next;
};

file_descriptor *new_fd(char *file_name, int perm, int file_descriptor, int file_pointer);

int have_f_write();

void renew_time(int fd);

int existed(int file_descriptor);

int name_exist(char *name);

file_descriptor *find_descriptor(int fd);

void add_fd(char *file_name, int perm, int file_descriptor, int file_pointer);

char *copy_char(char *target_list, int size);

int remove_fd(int fd);

#endif