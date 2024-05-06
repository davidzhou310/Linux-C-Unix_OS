#ifndef SHELL_H
#define SHELL_H

#include "process_list.h"

void shell();

int waitBg(process_node* job_head, process_node* job_tail);

void dummy_shell();

#endif