#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process_list.h"

// take in dummy head and dummy tail as stack variable
// in main execution loop and link the two dummy nodes
void initialize_list(process_node* head, process_node* tail) {
    head->jobId = 0;
    head->pgid = 0;
    head->numPids = 0;
    head->state = 0;
    head->cmd = NULL;

    tail->jobId = -1;
    tail->pgid = -1;
    tail->numPids = -1;
    tail->state = -1;
    tail->cmd = NULL;

    head->next = tail;
    tail->prev = head;
}

void append_process(process_node* head, process_node* tail, pid_t pgid, int numPids, int state, struct parsed_command* cmd) {
    process_node* origin = locateByGroup(head, tail, pgid);
    if (origin != NULL) {
        ++(origin->numPids);
        return;
    }

    process_node* newNode = malloc(sizeof(process_node));
    newNode->pgid = pgid;
    newNode->numPids = numPids;
    newNode->state = state;
    newNode->cmd = cmd;
    process_node* prev = tail->prev;
    newNode->jobId = prev->jobId + 1;

    prev->next = newNode;
    newNode->prev = prev;
    newNode->next = tail;
    tail->prev = newNode;
}

process_node* decreaseById(process_node* head, process_node* tail, pid_t pgid) {
    process_node* ptr = head->next;
    while (ptr != tail) {
        if (ptr->pgid == pgid) {
            break;
        }
        ptr = ptr->next;
    }
    --(ptr->numPids);
    return freeEmpty(ptr);
}

process_node* decreaseByPtr(process_node* head, process_node* tail, process_node* node) {
    --(node->numPids);
    return freeEmpty(node);
}

process_node* keepEmptyDecrease(process_node* head, process_node* tail, process_node* node) {
    --(node->numPids);
    if (node->numPids == 0) {
        node->state = P_EXITED;
    }
    return node;
}

process_node* freeEmpty(process_node* node) {
    if (node->numPids == 0) {
        discard(node);
        return NULL;
    }
    return node;
}

void discard(process_node* node) {
    process_node* prev = node->prev;
    process_node* next = node->next;
    prev->next = next;
    next->prev = prev;
    free(node->cmd);
    free(node);
}

void toTail(process_node* head, process_node* tail, process_node* node) {
    process_node* prev = node->prev;
    process_node* next = node->next;
    prev->next = next;
    next->prev = prev;

    prev = tail->prev;
    prev->next = node;
    node->prev = prev;
    node->next = tail;
    tail->prev = node;
}

process_node* findCurrent(process_node* head, process_node* tail) {
    process_node* ptr = tail->prev;
    process_node* lastRunning = NULL;
    while (ptr != head) {
        if (ptr->state == P_STOPPED) {
            return ptr;
        }
        if (lastRunning == NULL) {
            lastRunning = ptr;
        }
        ptr = ptr->prev;
    }
    return lastRunning;
}

process_node* locateByJob(process_node* head, process_node* tail, int jobId) {
    process_node* ptr = head->next;
    while (ptr != tail) {
        if (ptr->jobId == jobId) {
            return ptr;
        }
        ptr = ptr->next;
    }
    return NULL;
}

process_node* locateByGroup(process_node* head, process_node* tail, pid_t pgid) {
    process_node* ptr = head->next;
    while (ptr != tail) {
        if (ptr->pgid == pgid) {
            return ptr;
        }
        ptr = ptr->next;
    }
    return NULL;
}

void printAll(process_node* head, process_node* tail) {
    process_node* ptr = head->next;
    while (ptr != tail) {
        printJob(ptr);
        ptr = ptr->next;
    }
}

void printState(char* state, const struct parsed_command *const cmd) {
    fprintf(stderr, "%s: ", state);
    printCommand(cmd);
    fprintf(stderr, "\n");
}

void printJob(process_node* node) {
    fprintf(stderr, "[%d] ", node->jobId);
    printCommand(node->cmd);
    if (node->state == P_RUNNING) {
        fprintf(stderr, " (running)\n");
    }
    if (node->state == P_STOPPED) {
        fprintf(stderr, " (stopped)\n");
    }
}

void printCommand(const struct parsed_command *const cmd) {
    for (size_t i = 0; i < cmd->num_commands; ++i) {
        for (char **arguments = cmd->commands[i]; *arguments != NULL; ++arguments)
            fprintf(stderr, "%s ", *arguments);

        if (i == 0 && cmd->stdin_file != NULL)
            fprintf(stderr, "< %s ", cmd->stdin_file);

        if (i == cmd->num_commands - 1) {
            if (cmd->stdout_file != NULL)
                fprintf(stderr, cmd->is_file_append ? ">> %s " : "> %s ", cmd->stdout_file);
        } else fprintf(stderr, "| ");
    }
}

void discardAll(process_node* head, process_node* tail) {
    while (head->next != tail) {
        discard(head->next);
    }
}