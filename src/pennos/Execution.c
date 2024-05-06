#include <errno.h>
#include <error.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "Execution.h"
#include "Input.h"
#include "Utility.h"
#include "commands.h"
#include "file_system_call.h"
#include "system_call_helper.h"
#include "parser.h"
#include "builtin.h"

#include "user.h"
#include "osbuildin.h"

extern int ticks;
extern pid_t fg_pid;

void execute(char* input, process_node* job_head, process_node* job_tail) {
    struct parsed_command* cmd = NULL;
    int inputFd = STANDARD_IN;
    int outputFd = STANDARD_OUT;
    int i = parse_command(input, &cmd);
    pid_t pid = -1;
    
    if (i != 0) {
        f_write(STANDARD_OUT, "parse command", 14);
        return;
    }
    int buildInFlag = isBuiltIn(cmd->commands[0][0]);
    if (buildInFlag > 0) {
        pid = built_in(cmd, buildInFlag, job_head, job_tail);
        if (buildInFlag != NICE_FLAG && buildInFlag != EXECUTE_FILE_FLAG) {
            return;
        }
        if (buildInFlag == EXECUTE_FILE_FLAG) {
            file_cmd* fc = f_execute(cmd->commands[0][0]);
            openFd(cmd, &inputFd, &outputFd);
            int first_round = 0;
            for (int n = 0; n < fc->argv; n++) {
                if (first_round){
                    execute_f(fc->command_line[n], job_head, job_tail, inputFd, outputFd, cmd->stdout_file, 1);
                }
                else {
                    execute_f(fc->command_line[n], job_head, job_tail, inputFd, outputFd, cmd->stdout_file, 0);
                }
                first_round = 1;
            }
            return;
        }

    }
        //free(cmd);
    

    // open specified files

    openFd(cmd, &inputFd, &outputFd);



    if (buildInFlag == 0 && buildInFlag != NICE_FLAG)  {
        pid = p_spawn(spawn_child, cmd->commands[0], inputFd, outputFd);
        if (pid < 0) {
            p_perror("p_spawn");
        }
        import_parser(cmd, pid);
    }

    if (cmd->is_background) {
        fprintf(stderr, "[%d] %d\n", pid - 1, pid);
    }

    if (cmd->is_background) {
        append_process(job_head, job_tail, pid, 1, P_RUNNING, cmd);
    } else {
        fg_pid = pid;

        // wait for child exit
        int stopped = waitChild(cmd, job_head, job_tail, pid);
        if (stopped == -1) {
            free(cmd);
            return;
        }

        fg_pid = 0;
    }
}

void openFd(struct parsed_command* cmd, int* inputFd, int* outputFd) {
    int fd1, fd2;
    if (cmd->stdin_file != NULL) {
        int fd1 = f_open(cmd->stdin_file, F_READ);
        if (inputFd < 0) {
            f_write(STANDARD_OUT, "open", 5);
        } 
        inputFd = &fd1;
    }
    if (cmd->stdout_file != NULL) {
        if (cmd->is_file_append) {
            fd2 = f_open(cmd->stdout_file, F_APPEND);
            if (fd2 < 0){
                f_write(STANDARD_OUT, "open", 5);
            }
        } else {
            fd2 = f_open(cmd->stdout_file, F_WRITE);
            if (fd2 < 0){
                f_write(STANDARD_OUT, "open", 5);
            }
        }
        outputFd = &fd2;
    }
}

int waitChild(struct parsed_command* cmd, process_node* job_head, process_node* job_tail, int pid) {
    int stopped = 0;
    int wstatus = 0;
    pid_t cid = p_waitpid(pid, &wstatus, 0);
    if (cid < 0) {
        p_perror("p_waitpid");
    }
    if (W_WIFSTOPPED(wstatus)) {
        append_process(job_head, job_tail, pid, 1, P_STOPPED, cmd);
        stopped = 1;
    } else if (W_WIFSIGNALED(wstatus)) {
        fprintf(stderr, "\n");
    }

    if (stopped == 0) {
        free(cmd);
    } else {
        fprintf(stderr, "\n");
        printState("Stopped", cmd);
    }
    return stopped;
}


void execute_f(char* input, process_node* job_head, process_node* job_tail, int inputFd, int outputFd, char* out_file_name, int append) {
    struct parsed_command* cmd = NULL;
    int i = parse_command(input, &cmd);
    cmd->stdout_file = out_file_name;
    cmd->is_file_append = append;
    pid_t pid = -1;
    if (i != 0) {
        f_write(STANDARD_OUT, "parse command", 14);
        return;
    }
    int buildInFlag = isBuiltIn(cmd->commands[0][0]);
    if (buildInFlag > 0) {
        pid = built_in(cmd, buildInFlag, job_head, job_tail);
        if (buildInFlag != NICE_FLAG && buildInFlag != EXECUTE_FILE_FLAG) {
            return;
        }
        if (buildInFlag == EXECUTE_FILE_FLAG) {
            file_cmd* fc = f_execute(cmd->commands[0][0]);
            for (int n = 0; n < fc->argv; n++) {
                execute_f(fc->command_line[n], job_head, job_tail, inputFd, outputFd, cmd->stdout_file, 0);
            }
            return;
        }

    }

    if (buildInFlag == 0 && buildInFlag != NICE_FLAG)  {
        pid = p_spawn(spawn_child, cmd->commands[0], inputFd, outputFd);
        import_parser(cmd, pid);
    }

    if (cmd->is_background) {
        fprintf(stderr, "[%d] %d\n", ticks, pid);
    }

    if (cmd->is_background) {
        append_process(job_head, job_tail, pid, 1, P_RUNNING, cmd);
    } else {
        fg_pid = pid;

        // wait for child exit
        int stopped = waitChild(cmd, job_head, job_tail, pid);
        if (stopped == -1) {
            free(cmd);
            return;
        }

        fg_pid = 0;
    }
}