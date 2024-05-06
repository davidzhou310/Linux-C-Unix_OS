#ifndef KERNEL_H
#define KERNEL_H

#include "linked_list.h"
#include <ucontext.h>
#include <sys/types.h>
#include "system_call_helper.h"
#include "parser.h"

#define STACKSIZE 1638400

#define RUNNING 0
#define STOPPED 1
#define BLOCKED 2
#define ZOMBIED 3
#define SIGNALED 4
#define ORPHAN 5

#define FALSE 0
#define TRUE 1

#define S_SIGSTOP 0
#define S_SIGCONT 1
#define S_SIGTERM 2

extern list_node* running_node;
extern Pcb* running_pcb;
extern ucontext_t* active_context;
extern ucontext_t scheduler_context;
extern ucontext_t main_context;


typedef struct list_node list_node;
typedef struct Pcb Pcb;
typedef struct queues queues;

static const int centisecond = 10000;  // 10 milliseconds

struct queues {
    list_node* ready_queue_neg;
    list_node* ready_queue_zero;
    list_node* ready_queue_pos;

    list_node* blocked_queue;
    list_node* stopped_queue;
    list_node* zombie_queue;  // only store pid info

    list_node* all_process;
};

int initialize(void);           // initialize kernel context

void schedule(void);            // scheduler function

void idle(void);                // stay suspended until ready process available

// int check_waitable(pid_t pid);
Pcb* get_running(void);

void print_qs_pid(list_node* head);

void add_back_ready(list_node* node);

void check_sleeping_threads(void);

void set_idle();

void append_all_process_list(Pcb* pcb, list_node* backup);

list_node* find_which_queue (Pcb* pcb);
Pcb* find_pcb(pid_t pid);
void suspend_context(ucontext_t* ucp);

void check_how_finish(void);

void check_waited_by(void);

ucontext_t* get_scheduler_context(void);

ucontext_t* get_active_context(void);

Pcb* get_running_pcb(void);

list_node* get_running_node(void);

pid_t check_real_idle();

list_node* remove_node_iterate_all (pid_t pid);

list_node* get_backup(pid_t pid);

typedef struct list_node list_node;
typedef struct Pcb Pcb;
typedef struct queues queues;

// https://edstem.org/us/courses/32172/discussion/2802748
struct Pcb {
    ucontext_t context;
    pid_t pid;      // process id
    pid_t ppid;     // parent process id
    int priority;   // priority level
    int status;     // running/stopped/blocked/zombied
    int fd0;
    int fd1;
    int changed;
    int sleep_time;
    int neg_one_wait;
    int sigterm_flag;
    int fg;
    char** cmd;
    pid_t waited_by;
    int is_shell;
    struct parsed_command* parse;
    //list_node* waited;  // list of pids waiting on current process
    file_descriptor* file_descriptor;
};

extern queues qs;
extern ucontext_t scheduler_context;
extern ucontext_t* active_context;

Pcb* s_process_create(Pcb* parent);

// create a new child thread
Pcb* k_process_create(Pcb* parent);

void init_context(ucontext_t* ucp, int kernel);


void make_context(ucontext_t* ucp, void (*func)(), char* argv[]);

// kill thread process with signal signal
void k_process_kill(Pcb* process, int signal);

// clean up process status
void k_process_cleanup(Pcb* process);

void check_orphan_child(void);

int count_num_process_waitable();

void check_shell_exist();
#endif