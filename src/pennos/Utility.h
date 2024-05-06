#ifndef UTILITY_H
#define UTILITY_H

#define NEW_LINE 1

#include "parser.h"
#include "process_list.h"

extern process_node job_head;
extern process_node job_tail;
extern process_node* report;

// Override parent process interrupt handler (Ctrl-C).
void repromptHandler(int signum);
// Write prompt to stderr.
int prompt(int newLine);

#endif