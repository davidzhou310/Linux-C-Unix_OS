#ifndef EXECUTION_H
#define EXECUTION_H

#include "process_list.h"
#include "parser.h"

void execute(char* input, process_node* job_head, process_node* job_tail);

void openFd(struct parsed_command* cmd, int* inputFd, int* outputFd);

int waitChild(struct parsed_command* cmd, process_node* job_head, process_node* job_tail, int pid);

void execute_f(char* input, process_node* job_head, process_node* job_tail, int inputFd, int outputFd, char* out_file_name, int append);

#endif