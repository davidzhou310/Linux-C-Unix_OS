#include "osbuildin.h"
#include "user.h"
#include "kernel.h"
#include "shell.h"
#include <stdlib.h>
#include <string.h>
#include "stress.h"

#include "user_helper.h"
#include "file_system_call.h"


char* zc[1] = {"zombie_child"};
char* oc[1] = {"orphan_child"};
//all buildin function uc_link to cleanup context
void spawn_child (char* cmd[]) {
    if (strcmp(cmd[0], "sleep") == 0) {
            sleep_(atoi(cmd[1]));
    }
    else if (strcmp(cmd[0], "busy") == 0 ) {
        busy();
    }
    else if (strcmp(cmd[0], "ps") == 0 ) {
        ps();
    }
    else if (strcmp(cmd[0], "zombify") == 0 ) {
        zombify();
    }
    else if (strcmp(cmd[0], "orphanify") == 0 ) {
        orphanify();
    }
    else if (strcmp(cmd[0], "kill") == 0 ) {
        kill_(cmd);
    }
    else if (strcmp(cmd[0], "shell") == 0) {
        shell();
    }
    else if (strcmp(cmd[0], "cat") == 0) {
        char *stdin_file = get_stdin_file();
        char *stdout_file = get_stdout_file();
        if (stdin_file == NULL && stdout_file == NULL){
            cat(cmd[1], stdout_file, get_is_file_append());
        } else {
        cat(get_stdin_file(), get_stdout_file(), get_is_file_append());
        }
    }
    else if (strcmp(cmd[0], "echo") == 0) {
        echo(cmd, get_stdout_file(), get_is_file_append());
    }
    else if (strcmp(cmd[0], "ls") == 0) {
        if (cmd[1]) {
            f_ls(cmd[1]);
        } else {
            f_ls(NULL);
        }
    }
    else if (strcmp(cmd[0], "touch") == 0) {
        int idx = 1;
        while (cmd[idx]) {
            int fd = f_open(cmd[idx], F_WRITE);
            f_close(fd);
            idx++;
        }
    }
    else if (strcmp(cmd[0], "mv") == 0) {
        f_move(cmd[1], cmd[2]);
    }
    else if (strcmp(cmd[0], "cp") == 0) {
        copy_file(cmd[1], cmd[2]);
    }
    else if (strcmp(cmd[0], "rm") == 0) {
        remove_file(cmd);
    }
    else if (strcmp(cmd[0], "chmod") == 0) {
        chmod_file(cmd[1], cmd[2]);
    }
    else {
        perror("Command not exist!");
    }
}


void sleep_(int sec)
{
    p_sleep(sec);
}


void busy() {
    while (1);
}

void ps() {
    char log[100];
    sprintf(log, "PID\tPPID\tPRI\tSTAT\tCMD\n");
    f_write(STANDARD_OUT, log, strlen(log));
    list_node* head = qs.all_process->next;
    while (head != NULL) {
        if (head->pcb->status == RUNNING) {
            sprintf(log, "%d\t%d\t%d\t%s\t%s\n", head->pcb->pid, head->pcb->ppid, head->pcb->priority, "R", head->pcb->cmd[0]);
        }
        else if (head->pcb->status == STOPPED) {
            sprintf(log, "%d\t%d\t%d\t%s\t%s\n", head->pcb->pid, head->pcb->ppid, head->pcb->priority, "S", head->pcb->cmd[0]);
        }
        else if (head->pcb->status == BLOCKED) {
            sprintf(log, "%d\t%d\t%d\t%s\t%s\n", head->pcb->pid, head->pcb->ppid, head->pcb->priority, "B", head->pcb->cmd[0]);
        }
        else if (head->pcb->status == ZOMBIED || head->pcb->status == SIGNALED) {
            sprintf(log, "%d\t%d\t%d\t%s\t%s\n", head->pcb->pid, head->pcb->ppid, head->pcb->priority, "Z", head->pcb->cmd[0]);
        }
        else if (head->pcb->status == ORPHAN) {
            sprintf(log, "%d\t%d\t%d\t%s\t%s\n", head->pcb->pid, head->pcb->ppid, head->pcb->priority, "O", head->pcb->cmd[0]);
        }
        
        f_write(STANDARD_OUT, log, strlen(log));
        head = head->next;
    }
}

void ps_() {
    print_qs_pid(qs.zombie_queue);
    perror("---------------------------------");
    print_qs_pid(qs.blocked_queue);
    perror("---------------------------------");
    print_qs_pid(qs.stopped_queue);
    perror("---------------------------------");
    print_qs_pid(qs.ready_queue_zero);
    perror("---------------------------------");
    print_qs_pid(qs.all_process);
}




void zombie_child() {
    // MMMMM Brains...!
    return;
}
void zombify() {
    p_spawn(zombie_child, zc, 0, 1);
    while (1) ;
    return;
}

void orphan_child() {
    // Please sir,
    // I want some more
    while (1) ;
}
void orphanify() {
    p_spawn(orphan_child, oc, 0, 1);
    return;
}

void kill_(char* argv[]) {
    int i = 2;
    if (strcmp(argv[1], "-cont") == 0) {
        while (argv[i] != NULL && strcmp(argv[i], "\n") != 0 && strcmp(argv[i], "\0") != 0) {
            p_kill(atoi(argv[i]), S_SIGCONT);
            i++;
        }
    }
    else if (strcmp(argv[1], "-stop") == 0) {
        while (argv[i] != NULL && strcmp(argv[i], "\n") != 0 && strcmp(argv[i], "\0") != 0) {
            p_kill(atoi(argv[i]), S_SIGSTOP);
            i++;
        }
    }
    else if (strcmp(argv[1], "-term") == 0) {
        while (argv[i] != NULL && strcmp(argv[i], "\n") != 0 && strcmp(argv[i], "\0") != 0) {
            p_kill(atoi(argv[i]), S_SIGTERM);
            i++;

        }
    }
    /*else if (atoi(argv[2]) == 0) {
        //error
    }*/
    else {//default
        while (argv[i] != NULL && strcmp(argv[i], "\n") != 0 && strcmp(argv[i], "\0") != 0) {
            if (p_kill(atoi(argv[i]), S_SIGTERM) != 0) {
                p_perror("p_kill");
            }
            i++;
        }
    }
}

pid_t nice_(char* argv[]) {//need to be wait
    pid_t pid = -1;
    int i = 2;
    int pri = atoi(argv[1]);
    while (argv[i] != NULL && strcmp(argv[i], "\n") != 0 && strcmp(argv[i], "\0") != 0) {
        argv[i-2] = argv[i];
        i++;
    }
    argv[i-1] = NULL;
    argv[i-2] = NULL;
    pid = p_spawn(spawn_child, argv, 0, 1);
    if (p_nice(pid, pri) != 0) {
        p_perror("p_nice");
        return -1;
    }
    return pid;
}

pid_t nice_pid(char* argv[]) {
    int pri = atoi(argv[1]);
    pid_t pid = atoi(argv[2]);
    if(p_nice(pid, pri) != 0) {
        p_perror("p_nice");
        return -1;
    }
    return pid;
}

void logout() {
    exit_shell();
}

void man() {
    f_write(STANDARD_OUT, "cat\n", 5);
    f_write(STANDARD_OUT, "sleep\n", 7);
    f_write(STANDARD_OUT, "busy\n", 6);
    f_write(STANDARD_OUT, "echo\n", 6);
    f_write(STANDARD_OUT, "ls\n", 4);
    f_write(STANDARD_OUT, "touch\n", 7);
    f_write(STANDARD_OUT, "mv\n", 4);
    f_write(STANDARD_OUT, "cp\n", 4);
    f_write(STANDARD_OUT, "rm\n", 4);
    f_write(STANDARD_OUT, "chmod\n", 7);
    f_write(STANDARD_OUT, "ps\n", 4);
    f_write(STANDARD_OUT, "kill\n", 6);
    f_write(STANDARD_OUT, "zombify\n", 9);
    f_write(STANDARD_OUT, "orphanify\n", 11);
    f_write(STANDARD_OUT, "nice\n", 6);
    f_write(STANDARD_OUT, "nice_pid\n", 10);
    f_write(STANDARD_OUT, "man\n", 5);
    f_write(STANDARD_OUT, "bg\n", 4);
    f_write(STANDARD_OUT, "fg\n", 4);
    f_write(STANDARD_OUT, "jobs\n", 6);
    f_write(STANDARD_OUT, "logout\n", 8);
}