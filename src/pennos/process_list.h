#ifndef PROCESS_LIST_H
#define PROCESS_LIST_H

#include <sys/types.h>
#include <unistd.h>

#include "parser.h"

#define P_RUNNING 1
#define P_STOPPED 2
#define P_EXITED  3

#define FG_RUNNING 4
#define FG_STOPPED 5

typedef struct process_node process_node;

struct process_node {
    int jobId;
    pid_t pgid;
    int numPids;
    int state;
    struct parsed_command* cmd;

    process_node* prev;
    process_node* next;
};

// initialize dummyHead and dummyTail 
// in main loop and pass in as parameters
void initialize_list(process_node* head, process_node* tail);

// initialize newNode in main loop and
// append to the end of the linked list
void append_process(process_node* head, process_node* tail, pid_t pgid, int numPids, int state, struct parsed_command* cmd);

// remove the ListNode with specified pgid
process_node* decreaseById(process_node* head, process_node* tail, pid_t pgid);

// remove the ListNode with specified node pointer
process_node* decreaseByPtr(process_node* head, process_node* tail, process_node* node);

process_node* keepEmptyDecrease(process_node* head, process_node* tail, process_node* node);

// free heap memory if current group is empty
process_node* freeEmpty(process_node* node);

void discard(process_node* node);

void toTail(process_node* head, process_node* tail, process_node* node);

process_node* findCurrent(process_node* head, process_node* tail);

process_node* locateByJob(process_node* head, process_node* tail, int jobId);

process_node* locateByGroup(process_node* head, process_node* tail, pid_t pgid);

void printAll(process_node* head, process_node* tail);

void printState(char* state, const struct parsed_command *const cmd);

void printJob(process_node* node);

void printCommand(const struct parsed_command *const cmd);

void discardAll(process_node* head, process_node* tail);

#endif