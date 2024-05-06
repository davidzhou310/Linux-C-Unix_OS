#define _XOPEN_SOURCE 700

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <valgrind/valgrind.h>
#include "kernel.h"
#include "log.h"


queues qs;
Pcb* init;
Pcb* running_pcb = NULL;
list_node* running_node;
ucontext_t main_context;
ucontext_t scheduler_context;
ucontext_t idle_context;
ucontext_t* active_context = &scheduler_context;
struct itimerval it;
struct itimerval it_z;
struct sigaction act;
//sigset_t signal_mask;
list_node* running_node = NULL;
int finished_flag = 0;
static int max_pid = -1;
int ticks = 0;
//if finished flag = 1 the process swap to scheduler 
//because of alarm or other signal send
//if finished flag = 0 the process swap to scheduler 
//because of process finished normally

static void set_alarm(void);
static void alarm_handler(int signum);
static void set_timer(void);
static void init_queues(void);
static list_node* schedule_next(void);
static void set_stack(stack_t* stack);
static void set_timer_z(void);

// initialize kernel context
int initialize() {
    // register signals
    // signal(SIGINT, SIG_IGN);
    // signal(SIGTSTP, SIG_IGN);
    set_alarm();
    // call setitimer(2) to set time quanta
    //set_timer();
    // initialize queues
    init_queues();
    // initialize log file
    //init_log();
    // set random seed
    srandom(0);
    //init_log();
    //set_timer_z();
    //sigfillset(&signal_mask);
    // initialize init process
    init = s_process_create(NULL);
    //running_pcb = init;
    // initialize scheduler
    Pcb* scheduler_pcb = s_process_create(init);
    scheduler_context = scheduler_pcb->context;
    make_context(&scheduler_context, schedule, NULL);
    // swap in scheduler and remains idle
    set_idle();
    //setcontext(&scheduler_context);
    return 0;
}

static void set_alarm(void) {

    act.sa_handler = alarm_handler;
    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);

    sigaction(SIGALRM, &act, NULL);

    // for gdb debug purpose
    // struct sigaction act1;

    // act1.sa_handler = alarm_handler;
    // act1.sa_flags = SA_RESTART;
    // sigfillset(&act1.sa_mask);

    // sigaction(SIGPROF, &act1, NULL);
}

static void alarm_handler(int signum) {
    //("Alarm");
    finished_flag = 1;
    ticks++;
    //log_event(GENERAL, "ALARM", running_pcb, 0);
    swapcontext(active_context, &scheduler_context);
}


static void set_timer(void) {

    it.it_interval = (struct timeval) {.tv_usec = 0};
    //it.it_value = it.it_interval;
    it.it_value = (struct timeval) {.tv_usec = centisecond * 20};
    setitimer(ITIMER_REAL, &it, NULL);
    // setitimer(ITIMER_PROF, &it, NULL);
}

static void set_timer_z(void) {

    it_z.it_interval = (struct timeval) {.tv_usec = 0};
    it_z.it_value = it.it_interval;

    // setitimer(ITIMER_PROF, &it, NULL);
}

static void init_queues(void) {
    qs.ready_queue_neg = malloc(sizeof(list_node));
    qs.ready_queue_neg->next = NULL;
    qs.ready_queue_zero = malloc(sizeof(list_node));
    qs.ready_queue_zero->next = NULL;
    qs.ready_queue_pos = malloc(sizeof(list_node));
    qs.ready_queue_pos->next = NULL;

    qs.blocked_queue = malloc(sizeof(list_node));
    qs.blocked_queue->next = NULL;
    qs.stopped_queue = malloc(sizeof(list_node));
    qs.stopped_queue->next = NULL;
    qs.zombie_queue = malloc(sizeof(list_node));
    qs.zombie_queue->next = NULL;

    qs.all_process = malloc(sizeof(list_node));
    qs.all_process->next = NULL;
}

void schedule(void) {
    // reference Appendix A

    check_sleeping_threads();
    if (running_node != NULL && running_pcb != NULL && finished_flag == 0  &&  strcmp(running_pcb->cmd[0], "shell") != 0) {
        //check if ruuning_node finished normal, should go to zombie queue, 
        //if waited, waitpid will free it
        if (running_node->pcb->waited_by != -1) {
            //waited by other process, unblock process
            Pcb* waited_pcb = find_pcb(running_node->pcb->waited_by);
            waited_pcb->status = RUNNING;
            log_event(GENERAL, "UNBLOCKED", waited_pcb, 0);
            list_node* waited_node = remove_from_qs(qs.blocked_queue, waited_pcb);
            append_exist_node(find_which_queue(waited_pcb), waited_node);
            if (running_pcb->neg_one_wait == 1) {
            //if process is waited by waitpid(-1), change all other process
            // with -1 wait to unwaited
                list_node* all_head = qs.all_process;
                while (all_head->next != NULL) {
                    if (all_head->next->pcb->neg_one_wait == 1 
                    && all_head->next->pcb->waited_by == waited_pcb->pid) {
                        all_head->next->pcb->neg_one_wait = 0;
                        all_head->next->pcb->waited_by = -1;
                    }
                    all_head = all_head->next;
                }
            }
            running_node->pcb->waited_by = -1;
            running_node->pcb->neg_one_wait = 0;
        }
        running_pcb->status = ZOMBIED;
        running_pcb->changed = TRUE;
        log_event(GENERAL, "EXITED", running_pcb, 0);
        log_event(GENERAL, "ZOMBIE", running_pcb, 0);
        append_exist_node(qs.zombie_queue, running_node);
        running_node = NULL;
        running_pcb = NULL;
    }
    finished_flag = 0;

    check_orphan_child();
    check_shell_exist();

    if (running_node != NULL && running_node->pcb->status == RUNNING) {
        add_back_ready(running_node);
    }
    list_node* target = schedule_next();

    if (target != NULL){
        log_event(GENERAL, "SCHEDULE", target->pcb, 0);
        running_node = target;
        running_pcb = running_node->pcb;
        active_context = &(running_pcb->context);
        set_timer();
        setcontext(active_context);
    }
    else {//set to idle process
        pid_t mis_pid = check_real_idle();
        if (mis_pid != -1) {
            list_node* mis_node = get_backup(mis_pid);

            running_node = mis_node;
            running_pcb = mis_node->pcb;
            active_context = &(running_pcb->context);
            set_timer();
            setcontext(active_context);
        }
        else {
            running_node = NULL;
            running_pcb = NULL;
            active_context = &idle_context;
            set_timer();
            setcontext(&idle_context); 
        }

    }
}

static list_node* schedule_next(void) {
    // generate random number by calling random(3)
    float rand = (float) random() / (float) RAND_MAX;
    list_node* target = NULL;
    int id = 0;
    if (qs.ready_queue_neg->next != NULL) {
        id += 4;
    }
    if (qs.ready_queue_zero->next != NULL) {
        id += 2;
    }
    if (qs.ready_queue_pos->next != NULL) {
        id += 1;
    }
    if (id == 7) {
        if (rand < 0.4737) {
            target = qs.ready_queue_neg->next;
        } else if (rand < 0.7895) {
            target = qs.ready_queue_zero->next;

        } else {
            target = qs.ready_queue_pos->next;
        }
    }
    else if (id == 6) {
        if (rand < 0.6) {
            target = qs.ready_queue_neg->next;
        }
        else {
            target =qs.ready_queue_zero->next;
        }
    }
    else if (id == 5) {
        if (rand < 0.3077) {
            target = qs.ready_queue_pos->next;
        }
        else {
            target  = qs.ready_queue_neg->next;
        }
    }
    else if (id == 3) {
        if (rand < 0.6) {
            target = qs.ready_queue_zero->next;
        }
        else {
            target =qs.ready_queue_pos->next;
        }
    }
    else if (id == 4) {
        target = qs.ready_queue_neg->next;
    }
    else if (id == 2) {
        target = qs.ready_queue_zero->next;
    }
    else if (id == 1) {
        target = qs.ready_queue_pos->next;
    }

    if (target == NULL) {
        //if schedule idle process, running will point to none
        running_pcb = NULL;
    } else {
        
        target = remove_node(target->pcb);
        running_pcb = target->pcb;
    }  
    return target;
}

// stay suspended until ready available
void idle(void) {
    while (1) {
        suspend_context(&idle_context);
    }

}

void set_idle()
{
    init_context(&idle_context, 0);
    make_context(&idle_context, idle, NULL);
}

void suspend_context(ucontext_t* ucp) {
    sigemptyset(&(ucp->uc_sigmask));
    sigsuspend(&(ucp->uc_sigmask));
}

Pcb* get_running(void){
    return running_pcb;
}

void print_qs_pid(list_node* head) {
    list_node* cur = head;
    while (cur->next != NULL) {
        cur = cur->next;
    }
}

void add_back_ready(list_node* node) {
    int priority = node->pcb->priority;
    if (priority == 0) {
        append_exist_node(qs.ready_queue_zero, node);
    }
    else if (priority == -1) {
        append_exist_node(qs.ready_queue_neg, node);
    }
    else {
        append_exist_node(qs.ready_queue_pos, node);
    }
}

//check all sleeping threads in block queue, add back to ready queue after finish
//now we only consider sleep() function, should put in zombie queue
void check_sleeping_threads(void) {//need to implement special case for sleep()
    list_node* cur = qs.blocked_queue;
    while (cur->next != NULL) {
        if (cur->next->pcb->sleep_time > 1)
        {
            cur->next->pcb->sleep_time--;
            cur->next->pcb->sleep_time--;
            cur = cur->next;
        }
        else if (cur->next->pcb->sleep_time  == 1)
        {
            cur->next->pcb->sleep_time--;
            cur = cur->next;
        }
        else if (cur->next->pcb->sleep_time == 0)
        {
            cur->next->pcb->sleep_time = -1;
            cur->next->pcb->status = ZOMBIED;
            cur->next->pcb->changed =  TRUE;
            list_node* next = cur->next;
            cur = remove_from_qs(qs.blocked_queue, cur->next->pcb);
            log_event(GENERAL, "ZOMBIE", cur->pcb, 0);
            append_exist_node(qs.zombie_queue, cur);
            if (cur->pcb->waited_by != -1) { //sleeping is waited by other process
                Pcb* waited_pcb = find_pcb(cur->pcb->waited_by);
                waited_pcb->status = RUNNING;
                list_node* waited_node = remove_from_qs(qs.blocked_queue, waited_pcb);
                append_exist_node(find_which_queue(waited_pcb), waited_node);
                if (cur->pcb->neg_one_wait == 1) {
                //if process is waited by waitpid(-1), change all other process
                // with -1 wait to unwaited
                    list_node* all_head = qs.all_process;
                    while (all_head->next != NULL) {
                        if (all_head->next->pcb->neg_one_wait == 1 
                        && all_head->next->pcb->waited_by == waited_pcb->pid) {
                            all_head->next->pcb->neg_one_wait = 0;
                            all_head->next->pcb->waited_by = -1;
                        }
                        all_head = all_head->next;
                    }
                }
                cur->pcb->waited_by = -1;
                cur->pcb->neg_one_wait = 0;
            }
            cur = next;
        }
        else {
            cur = cur->next;
        }


    }
}

Pcb* find_pcb(pid_t pid) {
    list_node* cur = qs.all_process;
    while (cur->next != NULL && cur->next->pid != pid) {
        cur = cur->next;
    }
    if (cur->next == NULL) {
        return NULL;
    }
    else {
        return cur->next->pcb;
    }
}

void append_all_process_list(Pcb* pcb, list_node* backup) {
    list_node* node = malloc(sizeof(list_node));
    node->pcb = pcb;
    node->pid = pcb->pid;
    node->backup = backup;
    list_node* cur = qs.all_process;
    while (cur->next != NULL) {
        cur = cur->next;
    }
    cur->next = node;
    node->prev = cur;
    node->next = NULL;
}

list_node* find_which_queue (Pcb* pcb) {
    int cur_status = pcb->status;
    if (cur_status == BLOCKED) {
        return qs.blocked_queue;
    }
    else if (cur_status == STOPPED) {
        return qs.stopped_queue;
    }
    else if (cur_status == RUNNING) {
        if (pcb->priority == -1) {
            return qs.ready_queue_neg;
        }
        else if (pcb->priority == 0) {
            return qs.ready_queue_zero;
        }
        else if (pcb->priority == 1) {
            return qs.ready_queue_pos;
        }
    }
    else if (cur_status == ZOMBIED) {
        //NULL or we have an other queue for zombie process
        return qs.zombie_queue;
    }
    return NULL;
}

void check_waited_by(void) {
    //swap to waited_by context, set waited_by context to running queue and unblock it
    Pcb* waited_by_pcb = find_pcb(running_pcb->waited_by);
    waited_by_pcb->status = RUNNING;
    list_node* waited_by_node = remove_from_qs(qs.blocked_queue, waited_by_pcb);
    if (waited_by_pcb->priority ==  -1) {
        append_exist_node(qs.ready_queue_neg, waited_by_node);
    }
    else if (waited_by_pcb->priority == 0) {
        append_exist_node(qs.ready_queue_zero, waited_by_node);
    }
    else {
        append_exist_node(qs.ready_queue_pos, waited_by_node);
    }
    //do rest of waitpid before waitpid return
    //reason: if child is going to be terminated, we need to recode pid and status first
    active_context = &waited_by_pcb->context;
    swapcontext(&scheduler_context, active_context);
}

void check_how_finish(void) {
    if (finished_flag == 0 && running_node != NULL && running_pcb != NULL) {
        //process normal finish


        list_node* all_process_target = remove_from_qs(qs.all_process, running_pcb);
        free(running_node);
        free(running_pcb);
        free(all_process_target);
        running_node = NULL;
        running_pcb = NULL;
    }
}

list_node* get_running_node(void) {
    return running_node;
}

Pcb* get_running_pcb(void) {
    return running_pcb;
}

ucontext_t* get_active_context(void) {
    return active_context;
}

ucontext_t* get_scheduler_context(void) {
    return &scheduler_context;
}


//proces-----------------------

// create a new child thread
Pcb* s_process_create(Pcb* parent) {
    // initialize Pcb
    Pcb* pcb = malloc(sizeof(Pcb));
    
    // initialize context
    init_context(&pcb->context, 1);

    pcb->pid = max_pid++;
    if (parent != NULL) {
        pcb->ppid = parent->pid;
    }
    pcb->sleep_time = -1;

    // return pcb;
    return pcb;
}

// create a new child thread
Pcb* k_process_create(Pcb* parent) {
    // initialize Pcb
    Pcb* pcb = malloc(sizeof(Pcb));
    
    // initialize context
    init_context(&pcb->context, 0);
    pcb->pid = max_pid++;
    if (parent != NULL) {
        pcb->ppid = parent->pid;
    }
    else {
        pcb->ppid = -1;
    }
    pcb->priority = 0;
    pcb->status = RUNNING;
    pcb->fd0 = 0;
    pcb->fd1 = 1;
    pcb->sleep_time = -1;
    pcb->changed = FALSE;
    pcb->waited_by = -1;
    pcb->neg_one_wait = 0;
    pcb->sigterm_flag = 0;
    pcb->is_shell = 0;
    pcb->file_descriptor = NULL;
    // update ready queue
    //append(qs.ready_queue_zero, pcb);


    // append to all process list
    //append_all_process_list(pcb);

    // log process create
    // return pcb;
    //print_qs_pid(qs.ready_queue_zero);
    //sigprocmask(SIG_UNBLOCK, &act.sa_mask, NULL);
    return pcb;
}

void init_context(ucontext_t* ucp, int kernel) {

    getcontext(ucp);
    // initialize sigmask
    sigemptyset(&(ucp->uc_sigmask));
    // initialize stack
    set_stack(&(ucp->uc_stack));
    // set uc_link to scheduler context
    if (!kernel) {
        ucp->uc_link = &scheduler_context;
    }

}

static void set_stack(stack_t* stack) {
    stack_t* sp = malloc(STACKSIZE);
    VALGRIND_STACK_REGISTER(sp, sp + STACKSIZE);
    *stack = (stack_t) {.ss_sp = sp, .ss_size = STACKSIZE};
}

void make_context(ucontext_t* ucp, void (*func)(), char* argv[]) {

    if (argv != NULL) {

        makecontext(ucp, func, 1, argv);

    }
    else {
        makecontext(ucp, func, 0);
    }

}

// kill thread process with signal signal
void k_process_kill(Pcb* process, int signal) {
    list_node* origin_queue = find_which_queue(process);
    if (signal == S_SIGCONT) {//waited cases
        if (process->status != RUNNING && process->status != ZOMBIED) {
            //in practice, running node should not reach this point, 
            // it won't pass process->status check
            // so we don't consider the case processs->pid == running_node->pid
            process->status = RUNNING;
            process->changed = TRUE;
            log_event(GENERAL, "CONTINUED", process, 0);
            if (process->waited_by != -1) {
                //if process is waited, unblock waitpid
                Pcb* waited_by_pcb = find_pcb(process->waited_by);
                
                waited_by_pcb->status = RUNNING;
                list_node* waited_by_node = remove_from_qs(qs.blocked_queue, waited_by_pcb);
                add_back_ready(waited_by_node);
                if (process->neg_one_wait == 1)  {
                    //if process waited by waitpid(-1), reset all other -1 wait
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
                process->neg_one_wait = 0;
                process->waited_by = -1;
            }
            list_node* target = remove_from_qs(origin_queue, process);
            if (strcmp(process->cmd[0], "sleep_") == 0) {
                append_exist_node(qs.blocked_queue, target);
            }
            else if (process->priority == -1) {
                append_exist_node(qs.ready_queue_neg, target);
            }
            else if (process->priority == 0) {
                append_exist_node(qs.ready_queue_zero, target);
            }
            else if (process->priority == 1) {
                append_exist_node(qs.ready_queue_pos, target);
            }
        }
    }
    else if (signal == S_SIGSTOP) {//waited cases
        if (process->status != STOPPED && process->status != ZOMBIED) {
            process->status = STOPPED;
            process->changed = TRUE;
            log_event(GENERAL, "STOPPED", process, 0);
            if (process->waited_by != -1) {
                //if process is waited, unblock waitpid
                Pcb* waited_by_pcb = find_pcb(process->waited_by);
                waited_by_pcb->status = RUNNING;
                list_node* waited_by_node = remove_from_qs(qs.blocked_queue, waited_by_pcb);
                add_back_ready(waited_by_node);
                if (process->neg_one_wait == 1)  {
                    //if process waited by waitpid(-1), reset all other -1 wait
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
                process->waited_by = -1;
                process->neg_one_wait = 0;
            }
            if (running_node != NULL && process->pid == running_node->pid) {//if currently scheduled process is sigstop
                append_exist_node(qs.stopped_queue, running_node);
                finished_flag = 1;
                running_node = NULL;
                running_pcb = NULL;
                swapcontext(active_context, &scheduler_context);
            }
            else {//sigstop to process not scheduled
                list_node* target = remove_from_qs(origin_queue, process);
                append_exist_node(qs.stopped_queue, target);
            }
            
            
        }
    }
    else if (signal == S_SIGTERM) {
        process->status = SIGNALED;
        process->changed = TRUE;
        process->sigterm_flag  = 1;
        log_event(GENERAL, "SIGNALED", process, 0);
        if (strcmp(process->cmd[0], "shell") == 0) {//free shell
            if (process == running_pcb) {
                list_node* all_process_target = remove_from_qs(qs.all_process, process);
                free(process);
                free(running_node);
                free(all_process_target);
                running_node = NULL;
                running_pcb = NULL;
            }
            else {
                list_node* target = remove_from_qs(find_which_queue(process), process);
                list_node* all_process_target = remove_from_qs(qs.all_process, process);
                free(process);
                free(target);
                free(all_process_target);
            }
        }
        else if (process->waited_by != -1) {
            //case process is waited, unblock waiting process
            //free process when waitpid is running
            Pcb* waited_by_pcb = find_pcb(process->waited_by);
            waited_by_pcb->status = RUNNING;
            list_node* waited_by_node = remove_from_qs(qs.blocked_queue, waited_by_pcb);
            add_back_ready(waited_by_node);
            if (process->neg_one_wait == 1) {
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
            process->waited_by = -1;
            process->neg_one_wait = 0;
            if (running_node != NULL && process->pid == running_node->pid) {
                //if the running node is waited and get S_SIGTERM
                append_exist_node(qs.zombie_queue, running_node);
                running_node = NULL;
                running_pcb = NULL;
                finished_flag = 1;
                setcontext(&scheduler_context);
            }
            else {//waited but not running node get S_SIGTERM
                //find out process node and append to  zombie
                list_node* process_node = remove_from_qs(origin_queue, process);
                append_exist_node(qs.zombie_queue, process_node);
            }
            //process->waited_by = -1;
        }
        else {// process is not waited, we can free it now or turn to zombie
            if (running_node != NULL && process->pid == running_node->pid) {
                //scheduled process get S_SIGTERM
                append_exist_node(qs.zombie_queue, running_node);
                finished_flag = 1;
                setcontext(&scheduler_context);
            }
            else {//unscheduled and not waited process get S_SIGTERM, put to zombie queue
                //k_process_cleanup(process);
                list_node* target_node = remove_from_qs(origin_queue, process);
                append_exist_node(qs.zombie_queue, target_node);
            }
        }
        
    }

}

// clean up process status
void k_process_cleanup(Pcb* process) {
    // free heap memory
    // free ucontext_t
    // free Pcb
    // free list_node
    // update child status
    log_event(GENERAL, "EIXTED", process, 0);
    list_node* target = remove_from_qs(find_which_queue(process), process);
    list_node* all_process_target = remove_from_qs(qs.all_process, process);
    free(process);
    free(target);
    free(all_process_target);
}

void check_orphan_child(void) {
    list_node* head = qs.all_process->next;
    list_node* target = NULL;
    list_node* target_all_que = NULL;
    while (head != NULL) {
        pid_t ppid = head->pcb->ppid;
        if (ppid != -1 & head->pcb->status != ORPHAN) {
            Pcb* ppcb = find_pcb(ppid);
            if (ppcb == NULL) {
                log_event(GENERAL, "ORPHAN", head->pcb, 0);
                if (head->pcb == running_pcb) {
                    //append_exist_node(qs.zombie_queue, running_node);
                    target = running_node;
                    running_node = NULL;
                    running_pcb = NULL;
                }
                else {
                    target = remove_from_qs(find_which_queue(head->pcb), head->pcb);
                    //append_exist_node(qs.zombie_queue, target);
                }
                
                head->pcb->status = ORPHAN;
                //log
                head = head->next;
                target_all_que = remove_from_qs(qs.all_process, target->pcb);
                free(target->pcb);
                free(target);
                target = NULL;
                free(target_all_que);
                target_all_que = NULL;

            }
            else {
                head = head->next;
            }
        }
        else {
            head = head->next;
        }
        
    }
}

int count_num_process_waitable() {
    list_node* head = qs.all_process->next;
    int res = 0;
    while (head != NULL) {
        res++;
        head = head->next;
    }
    res--;
    return res;
}

void check_shell_exist() {
    list_node* head = qs.all_process->next;
    while (head != NULL && head->pcb->is_shell !=1) {
        head = head->next;
    }
    if (head == NULL) {
        exit(0);
    }
}

pid_t check_real_idle() {
    list_node* node = qs.all_process->next;
    while (node != NULL && node->pcb->status != RUNNING) {
        node = node->next;
    }
    if (node == NULL) {
        return -1;
    }
    else {
        return node->pid;
    }
}

list_node* remove_node_iterate_all (pid_t pid) {
    Pcb* pcb = find_pcb(pid);

    list_node* node = qs.blocked_queue->next;
    while (node != NULL && node->pcb != pcb) {
        node = node->next;
    }
    if  (node != NULL) {
        return node;
    }
    node = qs.zombie_queue->next;
    while (node != NULL && node->pcb != pcb) {
        node = node->next;
    }
    if (node != NULL) {
        return node;
    }
    node = qs.stopped_queue->next;
    while (node != NULL && node->pcb != pcb) {
        node = node->next;
    }
    if (node != NULL) {
        return node;
    }
    return NULL;
}

list_node* get_backup(pid_t pid) {
    list_node* node = qs.all_process->next;
    while (node != NULL && node->pid != pid) {
        node = node->next;
    }
    if (node != NULL) {
        return node->backup;
    }
    return NULL;
    
}