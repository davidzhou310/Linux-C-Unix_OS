#ifndef USER_H
#define USER_H

#include <sys/types.h>

#include <sys/time.h>
#include <stdio.h>
#include "parser.h"

extern int finished_flag;
// system calls

// process related system calls:

// fork(2)
pid_t p_spawn(void (*func) (), char* argv[], int fd0, int fd1);

// waitpid(2)
pid_t p_waitpid(pid_t pid, int* wstatus, int nohang);

// send sig to pid
int p_kill(pid_t pid, int sig);

// exit current thread unconditionally
void p_exit(void);

// macros:

// return true if child terminated normally
int W_WIFEXITED(int status);

// return true if child stopped
int W_WIFSTOPPED(int status);

// return true if child signalled
int W_WIFSIGNALED(int status);

// scheduling functions:

// set priority
int p_nice(pid_t pid, int priority);

// block calling thread
void p_sleep(unsigned int ticks);

void initiate_shell();

void go_scheuler_context ();

void set_ground(pid_t pid, int ground);

void check_move_bg(pid_t pid);

int get_fd0();

void import_parser(struct parsed_command* parse, pid_t pid);

char* get_stdin_file ();

char* get_stdout_file ();

int get_is_file_append();

int control_d_handler(int n, char *inputBuffer);

void set_shell();

void exit_shell();

pid_t p_getppid(pid_t pid);

#endif