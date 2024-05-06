#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "log.h"
#include "kernel.h"

static int fd;
extern int ticks;

void init_log(char * argv) {
    int flag = O_WRONLY | O_CREAT | O_TRUNC;
    if (argv == NULL) {
        fd = open("../log/log", flag, 0644);
    }
    else {
        char log_file[100];
        sprintf(log_file, "../log/%s", argv);
        fd = open(log_file, flag, 0644);
    }
    
}

void log_event(int event_type, char* operation, Pcb* pcb, int new_nice) {
    char log[100];
    switch(event_type) {
        case GENERAL:
        case SCHEDULE:
            sprintf(log, "[%04d]\t%-8s\t%d\t%d\t%s\n", ticks, operation, pcb->pid, pcb->priority, pcb->cmd[0]);
            break;
        case NICE:
            sprintf(log, "[%04d]\t%-8s\t%d\t%d\t%d\t%s\n", ticks, operation, pcb->pid, pcb->priority, new_nice, pcb->cmd[0]);
            break;
    }
    ssize_t numBytes = write(fd, log, strlen(log));
    int a = 1;
    // free(log);
}