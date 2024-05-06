#ifndef INPUT
#define INPUT

#include <sys/types.h>

#define BUFFER_SIZE 4096

#define BG_FLAG 1
#define FG_FLAG 2
#define JOBS_FLAG 3
#define NICE_FLAG 4
#define NICE_PID_FLAG 5
#define LOGOUT_FLAG 6
#define MAN_FLAG 7
#define HANG_FLAG 8
#define NOHANG_FLAG 9
#define RECUR_FLAG 10
#define EXECUTE_FILE_FLAG 11
#define MAN_FLAG 12

// Read user input into cmd.
ssize_t readInput(char* input);
// Count argument number to allocate memory.
int countArgument(char* input, ssize_t numBytes);
// Check if c is space.
int isSpace(char c);
// Return argument array to argValue.
void parseInput(char* input, int argCount, char** argValue);
// Return built-in type given input argument.
int isBuiltIn(char* input);

#endif