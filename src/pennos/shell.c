#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <strings.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "Execution.h"
#include "Input.h"
#include "process_list.h"
#include "Utility.h"
#include "commands.h"
#include "file_system_call.h"
#include "system_call_helper.h"
#include "parser.h"
#include "user.h"
#include "shell.h"
#include "stress.h"
#include "errno.h"

process_node job_head;
process_node job_tail;
process_node* report;

pid_t fg_pid;

void sigint_handler(int signo) {
    if (signo == SIGINT){
        if (fg_pid !=  0)
        {
            p_kill(fg_pid, 2);
        }
        else {
            f_write(STANDARD_OUT, "$\n", 3);
        }
        
    }
}

void sigtstp_handler(int signo)
{
    if (signo == SIGTSTP)
    {
        if (fg_pid != 0)
        {
            p_kill(fg_pid, 0);
        }
        else
        {
            f_write(STANDARD_OUT, "$\n", 3);

        }
    }
}
void sig_handler(){
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
}

void shell() {
    // char file_system;
    // strcpy(&file_system, "minfs");
    // do_mkfs(&file_system,1 , 0);
    char* input;

    initialize_list(&job_head, &job_tail);
    report = NULL;
    sig_handler();

    while (1) {
        char* input =(char*) malloc(BUFFER_SIZE);
        // prompt for user input
        int ret = prompt(0);
        if (ret == -1) {
            free(input);
            continue;
        }

        // get input
        int numBytes = f_read(STANDARD_IN, MAX_INPUT, input);
        // End command with null terminator.
        input[numBytes] = '\0';
        control_d_handler(numBytes, input);

        // check if exit
        if (numBytes == -1) {  
            // Ctrl-D pressed and input empty
            numBytes = f_write(STANDARD_OUT, "\n", sizeof(char));
            if (numBytes < 0){
                f_write(STANDARD_OUT, "write", 6);
            }
            discardAll(&job_head, &job_tail);
            free(input);
            break;
        }

        // wait for background jobs
        ret = waitBg(&job_head, &job_tail);
        if (ret == -1) {
            free(input);
            continue;
        }
        
        // parse input
        if (input[numBytes-1] != '\n') {
            // Ctrl-D pressed and input is not empty
            free(input);
            continue;
        }

        int argCount = countArgument(input, numBytes);
        if (argCount == 0) {
            // empty command
            free(input);
            continue;
        }

        execute(input, &job_head, &job_tail);
        free(input);
    }
}

int waitBg(process_node* head, process_node* job_tail) {
    process_node* ptr = head->next;
    while (ptr != job_tail) {
        int wstatus = -1;
        pid_t cid = 0;

        int numPids = ptr->numPids;
        for (int i = 0; i < numPids && ptr != NULL; ++i) {
            // non-blocking wait for finished child
            cid = p_waitpid(ptr->pgid, &wstatus, 1);
            if (W_WIFSTOPPED(wstatus)) {
                ptr->state = P_STOPPED;
                toTail(head, job_tail, ptr);
            }
            if (W_WIFEXITED(wstatus)) {
                if (ptr->numPids == 1) {
                    // job finished if last process exits
                    printState("Done", ptr->cmd);
                }
                ptr = decreaseByPtr(head, job_tail, ptr);
            }
            if ((wstatus == 0) && (ptr->state = P_STOPPED)) {
                ptr->state = P_RUNNING;
            }
        }
        if (cid > 0) { // if status changes
            if (ptr != NULL && ptr->state == P_STOPPED) {
                // alter: if (WIFSTOPPED(wstatus))
                printState("Stopped", ptr->cmd);
            }
            return 0;
        }
        ptr = ptr->next;
    }
    return 0;
}