#ifndef LOG_C
#define LOG_C

#define GENERAL 0
#define SCHEDULE 1
#define NICE 2

typedef struct Pcb Pcb;

void init_log(char* argv);

void log_event(int event, char* operation, Pcb* pcb, int old_nice);

#endif