#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "builtin.h"
#include "Input.h"
#include "kernel.h"
#include "process_list.h"
#include "user.h"
#include "Utility.h"
#include "osbuildin.h"
#include "stress.h"
#include "errno.h"

extern pid_t fg_pid;

int built_in(struct parsed_command* parsed, int built_in_flag, process_node* head, process_node* tail) {
    char** cmd = parsed->commands[0];
    int arg_count = sizeof(cmd) / sizeof(cmd[0]);
    if (built_in_flag == BG_FLAG || built_in_flag == FG_FLAG) {
        int job_id = 1;
        process_node* job = NULL;
        if (arg_count == 1) {
            // default without job_id
            job = findCurrent(head, tail);
        } else {
            // given job_id
            job_id = atoi(cmd[1]);
            job = locateByJob(head, tail, job_id);
        }
        if (built_in_flag == BG_FLAG) {
            return bg(head, tail, job);
        }
        if (built_in_flag == FG_FLAG) {
            if (job == NULL) {
                // checkError(-1, "fg");
                return -1;
            }
            return fg(head, tail, job);
        }
    }
    if (built_in_flag == JOBS_FLAG) {
        jobs_command(head, tail);
    }
    else if (built_in_flag == NICE_FLAG) {
        pid_t pid = nice_(cmd);
        import_parser(parsed, pid);
        return pid;
    }
    else if (built_in_flag == NICE_PID_FLAG) {
        pid_t pid = nice_pid(cmd);
    }
    else if (built_in_flag == HANG_FLAG) {
        hang();
    }
    else if (built_in_flag == NOHANG_FLAG) {
        nohang();
    }
    else if (built_in_flag == RECUR_FLAG) {
        recur();
    }
    else if (built_in_flag == LOGOUT_FLAG) {
        logout();
    }
    else if (built_in_flag == MAN_FLAG) {
        man();
    }
    return 0;
}

int bg(process_node* head, process_node* tail, process_node* job) {
    int killRet = p_kill(job->pgid, S_SIGCONT);
    if (killRet < 0) {
        p_perror("kill");
    }
    printState("Running", job->cmd);
    job->state = P_RUNNING;
    return 0;
}

int fg(process_node* head, process_node* tail, process_node* job) {
    if (job->state == P_STOPPED) {
        int killRet = p_kill(job->pgid, S_SIGCONT);
        if (killRet < 0) {
            p_perror("kill");
        }
        printState("Restarting", job->cmd);
        job->state = P_RUNNING;
    } else {
        printCommand(job->cmd);
        fprintf(stderr, "\n");
    }
    fg_pid = job->pgid;

    // wait for child exit
    int n = job->numPids;
    for (int i = 0; i < n; ++i) {
        int wstatus = 0;
        pid_t cid = p_waitpid(job->pgid, &wstatus, 0);
        if (W_WIFSTOPPED(wstatus)) {
            toTail(head, tail, job);
            fprintf(stderr, "\n");
            printState("Stopped", job->cmd);
            break;
        } else {
            if (W_WIFSIGNALED(wstatus)) {
                fprintf(stderr, "\n");
            }
            decreaseByPtr(head, tail, job);
        }
    }
    return 0;
}

void jobs_command(process_node* head, process_node* tail) {
    printAll(head, tail);
}

