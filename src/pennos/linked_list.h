#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <sys/types.h>


typedef struct list_node list_node;
typedef struct Pcb Pcb;

struct list_node {
    Pcb* pcb;
    pid_t pid;
    list_node* prev;    // can use the head.prev as a ptr (resulting in starvation)
    list_node* next;
    list_node* backup;
};
list_node* append(list_node* head, Pcb* pcb);

void to_tail(list_node* head);

list_node* query(list_node* head, pid_t pid);

list_node* remove_node(Pcb* target);

list_node* remove_from_qs(list_node* head, Pcb* target);

void append_exist_node(list_node* head, list_node* target);

#endif