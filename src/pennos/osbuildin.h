#ifndef OSBUILDIN
#define OSBUILDIN

#include "kernel.h"
#include "linked_list.h"
#include "user.h"

void test(char* argv[]);
void sleep_(int sec);
void busy();
void ps();
void ps_();
void zombie_child();
void zombify();
void orphan_child();
void orphanify();
void spawn_child (char* cmd[]);
void kill_(char* argv[]);
pid_t nice_pid(char* argv[]);
pid_t nice_(char* argv[]);
void logout();
void man();
#endif
