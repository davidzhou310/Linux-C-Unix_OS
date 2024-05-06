Group 35 Members: 

## Information
Haitian Zhou (PennKey: haitian); 
Jiayuan Chen (PennKey: jiayuanc);
Shuaijiang Liu (PennKey: shuaijl);
Jiahao Huang (PennKey: hjiahao);    

## Source Files
Source Files:
1. PennFAT
* `commands.c commands.h`
* `helper.c helper.h`
* `pennfat.c`

2. PennOS:  
* `builtin.c builtin.h`
* `commands_helper.c commands_helper.h`
* `commands.c commands.h`
* `errno.c errno.h`
* `Execution.c Execution.h`
* `file_system_call.c file_system_call.h `
* `Input.c Input.h`
* `kernel.c kernel.h`
* `linked_list.c linked_list.h `
* `log.c log.h `
* `os.c os.h`
* `osbuildin.c osbuild.h`
* `parser.c parser.h`
* `process_list.c process_list.h`
* `process.c process.h`
* `shell.c shell.h`
* `system_call_helper.c system_call_helper.h`
* `user_helper.c user_helper.h`
* `user.c user.h`
* `Utility.c Utility.h`

Extra Credit: None 

## Compilation
Please type in the following way: 
```make```->```cd ./bin```->```./pennfat```->```mkfs <file system name>  <num of file system block>  <block size config>```
->```CTRL_D```->```./pennos  <your file system name>```->commands

Explain: in the root directory you can only type ```make``` to compile the pennFAT program and pennOS program, and these two programs and its compile binary files is locate in directory ./bin. Therefore, you need to cd to bin directory before execute these program. You can also clean these compile file by running ```make clean```. After running make, if you only want to run pennFAT, you can ```cd ./bin```, then run ```./pennfat```. Then you can play around to the pennFAT.

## Overview
In this project, we have implemented PennFAT and PennOS. The PennOS is our own UNIX-like operating system. It includes programming a basic priority schedular, FAT file system, and user shell interactions.

There are two large parts of code. PennFAT is an independent part other than OS. OS can be divided into three major parts:
        Kernal/Schedular, PennFAT, and shell.

## Layout

### Layout for Standalone PennFAT 

For the PennFAT part, we have 5 files: commands.c and commands.h which implements each build-in function like mkfs, mount, cat -w; helper.c and helper.h which includes all helper functions for commands.c; pennfat.c which is the main function of pennfat. Starting from the main function in pennfat.c file, we used a while loop to take user input, parse it and check which commands the user exactly typed in and pass it to the corresponding handler in commands.c file. All of these three files have 3 global variables in total, the file descriptor for the file system fs_fd, the fat entry *fat, and the page size. The whole PennFAT commands and helper functions are at the very bottom level of the OS. The following is the elaboration of the detailed implementation of each function:

- mkfs:  do_mkfs() method. Creat a file system which looks like a single file on the host system. Use num_blocks and blocksize_config to calculate the FAT size and the data region size. Since mmap() function needs to be page aligned, we got the system page size using sysconf() and made it global. By using mmap() function to map an area of memory we calculated as our FAT. We also make the file descriptor of our FAT global. Then, store the size config of our file system in the first (index 0) FAT entry and FFFF in the second (index 1) FAT entry. 
- mount: Implemented by do_mount() method. open the fat file and maps the memory. Make *fat global. 
- umount : Implemented by do_umount(). munmap the FAT memory and close the file. 
- Touch: Implemented by do_touch() method. Check if the file exists by using locate_file_entry() helper function which returns its entry offset. If offset is -1, which means the file does not exist, call add_fat_entry() to allocate an empty fat entry for the file and mark it FFFF. The add_fat_entry returns the block num assigned to the file and then uses add_root_entry() function to add file_name, size, head_block, permission, type, time and 16 bits required for extra credit to the directory entry. If the root directory block is full, find an empty block for the new directory block and change FAT[1] value. If file exists, map the memory of file entry in the root directory by calculating the system page aligned offset. Renew the time in the entry and then munmap it.
- MV: Implemented by do_mv() method. Find the file entry in the root directory and change its name to the new name. If the new name has already been used, throw an error. 
- RM: Implemented by do_rm() method. Find the file entry in the root directory and change the first character of the file_name to 1, which stands for this file has been deleted so that when we need to create a new file, we can overwrite the information in the entry. If this file has been opened in other thread, change the first character of file_name to “2”. Then, we reset all indexes in the FAT by calling remove_fat_entry() and clear all data in its assigned data block by calling remove_data() method. 
- Cat -w file: implemented by do_cat_overwrite() function method. Read from STDIN and store in a buffer. Similar to touch,  if the file does not exist, we create a new file in the FAT and root directory, Then we use add_data_method() to add data in the buffer to the file. Each time we record how many bites we have written to the file and if it is smaller than the buffer size, we create a new block in the fat entry and keep writing the data. If the file exists, find the file entry, change its size and time, check the permission and use the similar method to add data. If the original file is longer than the current file size, we remove the exceeding data and fat entry. 
- Cat -a file: implemented by do_cat_append() method. Read from STDIN and store in a buffer. Find the end of the file by calling locate_end_of _file() method and then add data to the file and renew the size and time in the directory.
- Cat files..: implemented by do_cat() method. Find each file and print each character one by one to the terminal. 
- Cat files -w dest_file: implemented by do_concat_overwrite() method. Similar to do_cat_overwrite() method, but this time we use copy_data() function instead of add_data() method and keep appending it. 
- Cat files -a dest_file: implemented by do_append_overwrite() method. Similar to do_cat_append() but using copy_data() method. 
- LS: Implemented by do_ls() method. Using a for loop to traverse each file in all root directories and print it out in format. 
- CP file1 file2: Implemented by do_cp() method. Just call the do_concat_overwrite() method. 
- CP -h file1 file2: Implemented also by do_cp() method. Using getline() system call to read file1 from the host line by line and write it to the buffer. Then call the do_cat_overwrite() and do_cat_append() method to write to file2. 
- CP file1 -h file2: Implemented by do_cpto_host() method. Open file2 at host and find file1 and use write() method to write data block to file2. 
- Chmod: Implemented by do_chmod() method. We implement it by using bit operations. When it tries to add a mode, it uses the OR bit operator; when it tries to remove a mode, it uses AND operator to implement this.

### Layout for PennOS

For the kernel part, we have three files: `kernel.c`, `user.c`, and `linked_list.c`. In `kernel.c`, we developed kernel level system call and helper functions. User functions were implemented in `user.c`. `linked_list.c` contains the linked list node and helper functions that we use to maintain the priority queue. The implementation details are elaborated as follows:

* Kernel.c
  1. Pcb* k_process_create(Pcb* parent): the function will malloc a new pcb for the process we are creating and assign initial determination flags in Pcb. The function will return a new pcb for the process we are creating.
  2. void k_process_kill(Pcb* process, int signal): kill process to signal. 
      * Case 1: signal == S_SIGCONT, process will be put to ready based on its priority and set status to running. If process is already in ready queue or in zombie or orphan status, signal will be ignored.
      * Case 2: signal == S_SIGSTOP, process will be put to stopped queue and set status to stopped. If process is already in stopped queue or in zombie or orphan status, signal will be ignored.
      * Case 3: signal == S_SIGTERM, process will be put to zombie queue and set status to zombie. If process is already in zombie queue with status zombie or orphan, signal will be ignored.
  3. void k_process_cleanup(Pcb* process): the function will find linked list nodes that the pcb belongs to and remove these nodes from queues. Free pcb and all linked list node of the process.

* User.c:
  1. pid_t p_spawn(void (*func) (), char* argv[], int fd0, int fd1): In the function, it calls k_process_create to malloc new pcb, and do make context with u_context in new pcb finally append to default ready queue and all process list. If the function return success, it will return pid of new process. If fail, it will return -1.
  2. pid_t p_waitpid(pid_t pid, int* wstatus, int nohang): the function will check if desired pid process change status or not. If nothing can be wait or pid not exist, it will return -1 and throw an error. Otherwise, return waited pid. If the waited child is in zombie or orphan status, it will be free.
      * Case 1: pid == -1 and nohang == FALSE.  Function will check all children, if anyone change status, it will be waited and its pid will be returned. If no process change status, the thread calling p_waitpid will be blocked and swap to scheduler. The calling thread will be unblocked when any child change status. After it is scheduled, p_waitpid will find out which child change status and return child’s pid.
      * Case 2: pid == -1 and nohang == TURE. Function will check all children, if anyone change status, it will be waited and its pid will be returned. If no process change status, function will return -1.
      * Case 3: pid >0 and nohang ==FALSE. Function will find out the pcb of pid. If pid not exist, return -1. By checking pcb, we can know the process change status or not. If changed, process will be waited and return pid. If not changed, the thread calling p_waitpid will be blocked. The calling thread will be unblocked when the pid process change status. After the calling thread is scheduled, it will wait pid process and return pid.
      * Case 4: pid>0 and nohang == TRUE. Function will find out the pcb of pid. If pid not exist, return -1. If pid process change status, return pid, else return -1.
  3. int p_kill(pid_t pid, int sig): the function will call k_process_kill(). If pid not exist, return -1 and throw an error. Otherwise return 0.
void p_exit(): free current running thread immediately. If the process is waited, it will be put to zombie queue first. P_waitpid will free it. If it is not waited, it will be free in p_exist().
  4. int p_nice(pid_t pid, int priority): If pid not exist return -1. Otherwise return 0. Find pcb based on pid than set priority flag to priority. If process is in ready queue, move it to the ready queue of new priority level.
  5. void p_sleep (unsigned int ticks): calling thread will be set to blocked queue and BLOCKED status. A sleep timer is set up in running thread pcb. It will be deducted and check in scheduler. Last, swapcontext to scheduler_context.


1. Files for PennFAT: 
* `commands_helper.c` 
* `commands_helper.h` 
* `commands.c` 
* `commands.h`
* `file_system_call.c` 
* `file_system_call.h`
* `system_call_helper.c`
* `system_call_helper.h`

For the PennFAT part, we try to intergrate our code from standalone PennFAT code into PennOS. The `commands.c`, `commands.h`, `commands_helper.c`, and `commands_helper.h` is same as the standalone PennFAT part, and they are mainly define the build-in logic for pennfat. We define our file system related system calls in side `file_system_call.c`/`file_system_call.h`, these file system related system calls are based on `commands.h` (i.e. implemented by using the functions from `commands.h` and `commands_helper.h`). Therefore, user can only use these system calls to interact with binary file system in our PennOS.
        
2. Files for Kernal and Schedular:
* `kernel.c` 
* `kernel.h` 
* `linked_list.c` 
* `linked_list.h`
* `log.c` 
* `log.h`
* `os.c`
* `user.h`
* `user.c` 

This part of the code functionalities are already discussed in the previous section.

3. Files for shell: 
* `builtin.c` 
* `builtin.h`
* `Execution.c` 
* `Execution.h` 
* `Input.c` 
* `Input.h`
* `osbuildin.c` 
* `osbuildin.h` 
* `process_list.c` 
* `process_list.h`
* `shell.c` 
* `shell.h`
* `Utiltiy.c` 
* `Utiltiy.h`

The shell part is based on the previous project implementation. The `shell.c` consists of functions responsible for the main loop taking in commands and parsing them. `shell.c` will call `Execution.c` with parsed input to actually run the command. If certain commands are builtin commands which do not need to spawn a process, `execute` function in `Execution.c` will then call functions in `builtin.c` to process the command. Otherwise, it will spawn a child and go to functions in `osbuildin.c` to execute.