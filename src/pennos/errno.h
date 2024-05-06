#ifndef ERRNO
#define ERRNO

#define SUCCESS 0
#define MULTIPLE_WRITE 1
#define FAT_FULL 2
#define PERMISSION 3
#define INVALID_COMMAND 4
#define INVALID_FD 5
#define PID 6
#define INVALID_MOD 7
#define FAILURE 8
#define SIG_ERROR 9

void p_perror(char *message);

#endif
