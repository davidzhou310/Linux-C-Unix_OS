#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "user.h"
#include "osbuildin.h"
#include "kernel.h"
#include "shell.h"
#include "file_system_call.h"
#include "log.h"


int main(int argc, char* argv[]) 
{
    if (argc > 2) {
        init_log(argv[2]);
    }
    else {
        init_log(NULL);
    }
    initialize();
    if (f_mount(argv[1]) < 0){
        f_write(STANDARD_OUT, "mount", 6);
        // exit(EXIT_FAILURE);
    }
    char** shell_name = (char **) malloc(sizeof(char *) * 1);
    char * shell_str = (char *) malloc(sizeof(char) * 6); 
    shell_str[0] = 's';
    shell_str[1] = 'h';
    shell_str[2] = 'e';
    shell_str[3] = 'l';
    shell_str[4] = 'l';
    shell_name[0] = shell_str;

    pid_t shell_pid = p_spawn(spawn_child, shell_name, 0, 1);
    set_shell();
    go_scheuler_context();
    
}

