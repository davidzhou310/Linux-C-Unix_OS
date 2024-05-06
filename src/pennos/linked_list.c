#include <stdio.h>
#include <stdlib.h>

#include "linked_list.h"
#include "kernel.h"

typedef struct queues queues;
extern queues qs;

list_node* append(list_node* head, Pcb* pcb) {
    list_node* new_node = malloc(sizeof(list_node));


    if (head->next == NULL) {
        head->next = new_node;
        new_node->prev = head;
        new_node->next = NULL;
    }
    else {
        list_node* next = head->next;
        head->next = new_node;
        new_node->prev = head;
        new_node->next = next;
        next->prev = new_node;
    }
    new_node->pcb = pcb;
    new_node->backup = NULL;
    new_node->pid = pcb->pid;
    return new_node;

}

void to_tail(list_node* head) {
    list_node* curr_pcb = head->next;
    if (curr_pcb->next == NULL) {
        return;     // only one process
    }
    list_node* next_pcb = curr_pcb->next;
    list_node* last_pcb = curr_pcb->next;
    while (last_pcb->next != NULL) {
        last_pcb = last_pcb->next;
    }
    head->next = next_pcb;
    next_pcb->prev = head;
    last_pcb->next = curr_pcb;
    curr_pcb->prev = last_pcb;
    curr_pcb->next = NULL;
}

list_node* query(list_node* head, pid_t pid) {
    list_node* node = head;
    while (node->next != NULL) {
        if (node->pcb->pid == pid) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

list_node* remove_node(Pcb* target){
    int priority = target->priority;
    list_node* cur = NULL;
    if (priority == -1){
        cur = remove_from_qs(qs.ready_queue_neg, target);
    }
    else if (priority == 0){
        cur = remove_from_qs(qs.ready_queue_zero, target);
    }
    else if (priority == 1) {
        cur = remove_from_qs(qs.ready_queue_pos, target);
    }
    return cur;
}

list_node* remove_from_qs(list_node* head, Pcb* target){
    list_node* cur = head->next;
    pid_t target_pid = target->pid;
    while (cur != NULL && target_pid != cur->pid)
    {
        cur = cur->next;
    }
    if (cur != NULL)
    {
        list_node* prev = cur->prev;
        prev->next = cur->next;
        if (cur->next != NULL) {
            cur->next->prev = prev;
        }
        cur->prev = NULL;
        cur->next = NULL;
    }
    return cur;
}

void append_exist_node(list_node* head, list_node* target){
    list_node* cur = head;
    while (cur->next != NULL){
        cur = cur->next;
    }
    cur->next = target;
    target->prev = cur;
    target->next = NULL;
    
}

