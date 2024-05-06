#include "user.h"
#include "kernel.h"
#include <stdlib.h>
#include "log.h"
#include "parser.h"
#include <string.h>
#include <stdio.h>
#include "errno.h"

extern int error_code;

// fork(2)
pid_t p_spawn(void (*func) (), char* argv[], int fd0, int fd1) {
    // call k_process_create() with current running Pcb
    Pcb* pcb = k_process_create(running_pcb);
    pcb->fd0 = fd0;
    pcb->fd1 = fd1;
    pcb->cmd = argv;
    log_event(GENERAL, "CREATE",pcb, 0);
    make_context(&pcb->context, func, argv);
    list_node* node = append(qs.ready_queue_zero, pcb);
    append_all_process_list(pcb, node);
    return pcb->pid;
}

// waitpid(2)
pid_t p_waitpid(pid_t pid, int* wstatus, int nohang) {
    if (count_num_process_waitable() == 0) {
        return -1;//no more process can be wait
    }
    if (pid == -1) {//wait any child change status
        if (!nohang) {
            list_node* cur = qs.all_process;
            while (cur->next != NULL && cur->next->pcb->changed != TRUE) {
                /*if (strcmp(cur->next->pcb->cmd[0], "shell") != 0) {
                    cur->next->pcb->neg_one_wait = 1;
                    cur->next->pcb->waited_by = running_node->pid;
                    cur = cur->next;
                }*/
                cur->next->pcb->neg_one_wait = 1;
                cur->next->pcb->waited_by = running_node->pid;
                cur = cur->next;
                if (cur->next == NULL) {
            //have run one round of all child, no one change status
            //set cur to all_process head and blocked waitpid.
                    cur = qs.all_process;
                    running_pcb->status = BLOCKED;
                    append_exist_node(qs.blocked_queue, running_node);
                    finished_flag = 1;
                    log_event(GENERAL, "BLOCKED", running_pcb, 0);
                    swapcontext(active_context, &scheduler_context);
                }
            }

            //after child  change status, waitpid is unblocked, 
            //if child  change  to  zombie, clean it.

            //zombie queue situation
            Pcb* target_pcb = cur->next->pcb;
            log_event(GENERAL, "WAITED", target_pcb, 0);
            target_pcb->changed = FALSE;
            target_pcb->waited_by = -1;
            if (wstatus != NULL) {
                if (target_pcb->status == RUNNING) {
                    *wstatus = RUNNING;
                }
                else if (target_pcb->status == STOPPED) {
                    *wstatus = STOPPED;
                }
                else if (target_pcb->status == BLOCKED) {
                    *wstatus = BLOCKED;
                }
                else {
                    *wstatus = ZOMBIED;
                }
            }
            pid_t cur_pid = cur->next->pcb->pid;
            //now we don't consider the situation that a not scheduled child
            //get sigterm and is waited by some process
            Pcb* target = cur->next->pcb;
            if (target->status == ZOMBIED || target->status == SIGNALED) {
                list_node* free_node_1 = remove_from_qs(qs.all_process, target);
                list_node* free_node_2 = remove_from_qs(qs.zombie_queue, target);
                free(target);
                free(free_node_1);
                free(free_node_2);
            }
            //
            return cur_pid;
        }
        else {//return immediately
            list_node* cur = qs.all_process;
            while (cur->next != NULL && cur->next->pcb->changed == FALSE) {
                cur = cur->next;
            }
            if (cur->next == NULL) {
                return 0;
            }
            else {
                cur->next->pcb->changed = FALSE;
                if (wstatus != NULL) {
                    if (cur->next->pcb->status == RUNNING) {
                        *wstatus = RUNNING;
                    }
                    else if (cur->next->pcb->status == STOPPED) {
                        *wstatus = STOPPED;
                    }
                    else if (cur->next->pcb->status == BLOCKED) {
                        *wstatus = BLOCKED;
                    }
                    else {
                        *wstatus = ZOMBIED;
                    }
                }
                pid_t return_pid = cur->next->pid;
                Pcb* target = cur->next->pcb;
                log_event(GENERAL, "WAITED", target, 0);
                if (target->status == ZOMBIED || target->status == SIGNALED) {
                    print_qs_pid(qs.zombie_queue);
                    list_node* free_node_1 = remove_from_qs(qs.all_process, target);
                    list_node* free_node_2 = remove_from_qs(qs.zombie_queue, target);
                    free(target);
                    free(free_node_1);
                    free(free_node_2);
                }
                return return_pid;
            }
        }

    }
    //set calling thread to blocked
    Pcb * target = NULL;
    //pcb * temp = NULL;
    if (!nohang)//block calling process untill child change status
    {
        target = find_pcb(pid);//find target pid, need to refine find_pid function
        if (target != NULL)//pid exist
        {
            //while return when calling process is unblock
            //child change status, check it is wait by process or not
            while (target->changed == FALSE)
            {
                Pcb * calling_thread = running_pcb;
                target->waited_by = calling_thread->pid;
                calling_thread->status = BLOCKED;
                append_exist_node(qs.blocked_queue, running_node);
                finished_flag = 1;
                log_event(GENERAL, "BLOCKED", running_pcb, 0);
                swapcontext(active_context, &scheduler_context);
            }
            //swap back from scheduler when waiting child change  status
            log_event(GENERAL, "WAITED", target, 0);
            target->changed = FALSE;
            target->waited_by = -1;
            if (wstatus != NULL) {
                if (target->status == RUNNING) {
                    *wstatus = RUNNING;
                }
                else if (target->status == STOPPED) {
                    *wstatus = STOPPED;
                }
                else if (target->status == BLOCKED) {
                    *wstatus  = BLOCKED;
                }
                else {
                    *wstatus = ZOMBIED;
                }
            }
            if (target->status == ZOMBIED || target->status == SIGNALED) {
                list_node* free_node_1 = remove_from_qs(qs.all_process, target);
                list_node* free_node_2 = remove_from_qs(qs.zombie_queue, target);
                free(target);
                free(free_node_1);
                free(free_node_2);
            }
            return pid;
        }
        else//pid not exist
        {
            error_code = PID;
            return -1;
        }
    }
    else//return immediately
    {
        target = find_pcb(pid);
        if (target != NULL && target->changed == TRUE)
        {
            log_event(GENERAL, "WAITED", target, 0);
            target->changed = FALSE;
            if (wstatus != NULL) {
                if (target->status == RUNNING) {
                    *wstatus = RUNNING;
                }
                else if (target->status == STOPPED) {
                    *wstatus = STOPPED;
                }
                else if (target->status == BLOCKED) {
                    *wstatus  = BLOCKED;
                }
                else {
                    *wstatus = ZOMBIED;
                }
            }
            if (target->status == ZOMBIED || target->status == SIGNALED) { // if finished clean target node
                list_node* free_node_1 = remove_from_qs(qs.all_process, target);
                list_node* free_node_2 = remove_from_qs(qs.zombie_queue, target);
                free(target);
                free(free_node_1);
                free(free_node_2);
            }
            return pid;
        }
        else
        {
            return -1;
        }
    }

    return -1;
}

// send sig to pid
int p_kill(pid_t pid, int sig) {
    Pcb* target = find_pcb(pid);
    if (target != NULL) {
        k_process_kill(target, sig);
    }
    else {
        error_code = PID;
        return -1;
    }
    return 0;
}

// exit current thread unconditionally
void p_exit(void) {
    //process is waiting by other process, ublock waiting process, set running process to zombie, free it when running waiting process get called
    if (running_pcb->waited_by != -1) {
        Pcb* waited_by_pcb = find_pcb(running_pcb->waited_by);
        waited_by_pcb->status = RUNNING;
        list_node* waited_by_node = remove_from_qs(qs.blocked_queue, waited_by_pcb);
        add_back_ready(waited_by_node);
        if (running_pcb->neg_one_wait == 1) {
            //if process is waited by waitpid(-1), change all other process
            // with -1 wait to unwaited
            list_node* all_head = qs.all_process;
            while (all_head->next != NULL) {
                if (all_head->next->pcb->neg_one_wait == 1 
                && all_head->next->pcb->waited_by == waited_by_pcb->pid) {
                    all_head->next->pcb->neg_one_wait = 0;
                    all_head->next->pcb->waited_by = -1;
                }
                all_head = all_head->next;
            }
        }
        //set running_node to zombied queue, it will be freed when waitpid function be scheduled.
        running_pcb->status = ZOMBIED;
        append_exist_node(qs.zombie_queue, running_node);
        running_node = NULL;
        running_pcb = NULL;
    }
    else { //running_pcb is not waited by other process, terminate directly
        list_node* all_process_target  = remove_from_qs(qs.all_process, running_pcb);
    //Pcb* target_pcb = running_pcb;
    //list_node* target_node = running_node;
        free(all_process_target);
        free(running_pcb);
        free(running_node);
        running_node = NULL;
        running_pcb = NULL;
        finished_flag = 1;
        setcontext(&scheduler_context);
    }

}

// set priority
int p_nice(pid_t pid, int priority) {
    // look up Pcb instance
    // update priority value
    // update ready queue
    Pcb* pcb = find_pcb(pid);
    if (pcb == NULL) {
        error_code = PID;
        return -1;
    }
    int old_nice = pcb->priority;
    
    if (pcb->status == RUNNING) {
        list_node* node = remove_from_qs(find_which_queue(pcb), pcb);
        if (priority == -1) {
            append_exist_node(qs.ready_queue_neg, node);
        }
        else if (priority == 0) {
            append_exist_node(qs.ready_queue_zero, node);
        }
        else {
            append_exist_node(qs.ready_queue_pos, node);
        }
    }
    log_event(NICE, "NICE", pcb, priority);
    pcb->priority = priority;
    return 0;
}

// block calling thread
void p_sleep(unsigned int ticks) {
    running_pcb->status = BLOCKED;
    running_pcb->sleep_time = ticks*10;
    log_event(GENERAL, "BLOCKED", running_pcb, 0);
    append_exist_node(qs.blocked_queue, running_node);
    list_node* cur = running_node;
    running_node = NULL;
    running_pcb = NULL;
    finished_flag = 1;
    swapcontext(&cur->pcb->context, &scheduler_context);

}

int W_WIFEXITED(int status) {
    if (status == ZOMBIED) {
        return 1;
    }
    return 0;
}

int W_WIFSIGNALED(int status) {
    if (status == SIGNALED) {
        return  1;
    }
    return 0;
}

int W_WIFSTOPPED(int status) {
    if (status == STOPPED) {
        return 1;
    }
    return 0;
}

void initiate_shell() {
    initialize();
}

void go_scheuler_context () {
    swapcontext(&main_context, &scheduler_context);
}

void set_ground(pid_t pid, int ground) {
    Pcb* pcb = find_pcb(pid);
    pcb->fg = ground;
}
void check_move_bg(pid_t pid) {
    Pcb* pcb = find_pcb(pid);
    if (pcb->fg == 0 && pcb->fd0 == STANDARD_IN) {
        pcb->status = STOPPED;
        pcb->changed = TRUE;
        list_node* node = remove_from_qs(find_which_queue(pcb), pcb);
        append_exist_node(qs.stopped_queue, node);
    }
}


int get_fd0() {
    return running_pcb->fd0;
}

void import_parser(struct parsed_command* parse, pid_t pid) {
    Pcb* pcb = find_pcb(pid);
    pcb->parse = parse;
}

char* get_stdin_file () {
    return running_pcb->parse->stdin_file;
}

char* get_stdout_file () {
    return running_pcb->parse->stdout_file;
}

int get_is_file_append() {
    return running_pcb->parse->is_file_append;
}

int control_d_handler(int n, char *inputBuffer){
    if (n == 0 && inputBuffer[0] != '\n'){
        exit(0);
        }
    else if (n > 1 && inputBuffer[n - 1] != '\n'){
        write(STANDARD_OUT, "\n", sizeof("\n"));
        return 1;
        }
    return 0;
}

void set_shell() {
    qs.all_process->next->pcb->is_shell = 1;
    p_nice(1, -1);
}

void exit_shell() {
    list_node* head = qs.all_process->next;
    while (head->pcb->is_shell != 1) {
        head = head->next;
    }
    if (running_pcb != head->pcb) {
        list_node* node = remove_from_qs(find_which_queue(head->pcb), head->pcb);
        head = remove_from_qs(qs.all_process, head->pcb);
        free(node);
        free(head);
        free(head->pcb);
    }
    else {
        head = remove_from_qs(qs.all_process, head->pcb);
        free(head);
        free(running_node);
        free(running_pcb);
        running_node = NULL;
        running_pcb = NULL;
    }

    
}

pid_t p_getppid(pid_t pid) {
    Pcb* pcb = find_pcb(pid);
    return pcb->ppid;
} 