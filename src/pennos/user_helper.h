#ifndef USER_HELPER
#define USER_HELPER

#define NREAD 1024

#define F_WRITE -1
#define F_READ 0
#define F_APPEND 1

#define STANDARD_IN 0
#define STANDARD_OUT 1

void move_file(char *src, char *dest);

void copy_file(char *src, char *dest);

void remove_file(char *files[]);

void chmod_file(char *mode, char *file);

void cat(char *stdin_file, char *stdout_file, int is_file_append);

void echo(char *cmd[], char *stdout_file, int is_file_append);

#endif