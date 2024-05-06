
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>

#include "file_system_call.h"
#include "commands.h"
#include "commands_helper.h"
#include "system_call_helper.h"

int f_open(const char *fname, int mode){
    int offset = locate_file_entry(fname);
    switch (mode) {
        case F_WRITE:
            if (have_f_write() == 1){
                error_code = 1;
                return -1;
            }
            if (offset == -1){
                int first_block = add_fat_entry();
                if (first_block == -1){
                    error_code = 2;
                    return -1;
                }
                add_root_entry(fname, first_block, 0, REGULAR, RDWR);
                add_fd(fname, mode, first_block, 0);
                return first_block;
            } else {
                int directory_block = find_block_index(offset);
                int file_entry_index = find_entry_index(offset, directory_block);
                int directory_block_offset = get_block_offset(directory_block);
                int directory_pa_offset = get_pa_offset(directory_block_offset);
                void * directory_pa_block = pa_block_map(directory_pa_offset);
                file_entry *file_directory = &directory_pa_block[directory_block_offset - directory_pa_offset];
                file_entry *cur_file_entry = &file_directory[file_entry_index];
                int fd = cur_file_entry->first_block;
                if (!can_write_file(fname)){
                    error_code = 5;
                    return -1;
                }
                remove_data(0, fd);
                remove_fat_entry(fd);
                cur_file_entry->size = 0;
                cur_file_entry->mtime = time(NULL);
                fat[fd] = MAX_INDEX;
                add_fd(fname, mode, fd, 0);
                pa_block_unmap(directory_pa_block, directory_pa_offset);
                return fd;
            }
        case F_READ:
            if (offset == -1){
                error_code = 5;
                return -1;
            }
            int directory_block = find_block_index(offset);
            int file_entry_index = find_entry_index(offset, directory_block);
            int directory_block_offset = get_block_offset(directory_block);
            int directory_pa_offset = get_pa_offset(directory_block_offset);
            void * directory_pa_block = pa_block_map(directory_pa_offset);
            file_entry *file_directory = &directory_pa_block[directory_block_offset - directory_pa_offset];
            file_entry *cur_file_entry = &file_directory[file_entry_index];
            if (!can_read_file(fname)){
                error_code = 3;
                return -1;
            }
            int fd = cur_file_entry->first_block;
            cur_file_entry->mtime = time(NULL);
            add_fd(fname, mode, fd, 0);
            pa_block_unmap(directory_pa_block, directory_pa_offset);
            return fd;
        case F_APPEND:
            if (offset == -1){
                error_code = 5;
                return -1;
            }
            directory_block = find_block_index(offset);
            file_entry_index = find_entry_index(offset, directory_block);
            directory_block_offset = get_block_offset(directory_block);
            directory_pa_offset = get_pa_offset(directory_block_offset);
            directory_pa_block = pa_block_map(directory_pa_offset);
            file_directory = &directory_pa_block[directory_block_offset - directory_pa_offset];
            cur_file_entry = &file_directory[file_entry_index];
            if (!can_write_file(fname)){
                error_code = 3;
                return -1;
            }
            fd = cur_file_entry->first_block;
            cur_file_entry->mtime = time(NULL);
            int end_of_file = locate_end_of_file(cur_file_entry);
            int file_pointer = (end_of_file - get_fat_size()) % get_block_size(get_config());
            add_fd(fname, mode, fd, file_pointer);
            pa_block_unmap(directory_pa_block, directory_pa_offset);
            return fd;
        default:
            error_code = 7;
            return -1;
    }
}

int f_read(int fd, int n, char *buf){
    if (fd == STANDARD_IN){
        int count = read(STDIN_FILENO, buf, n);
        return count;
    }
    int filled = 0, is_end = 0, index = fd;
    int block_size = get_block_size(get_config());
    file_descriptor *node = find_descriptor(fd);
    int start_at = node->file_pointer;
    renew_time(fd);
    if (node->perm != F_READ){
        error_code = 3;
        return -1;
    }
    int block_offset = get_block_offset(fd);
    while (filled < n){
        int pa_offset = get_pa_offset(block_offset);
        void *pa_block = pa_block_map(block_offset);
        char *block = &pa_block[block_offset - pa_offset];
        while (start_at < block_size && filled < n){
            buf[filled] = block[start_at];
            if (buf[filled] == '\0'){
                is_end = 1;
                start_at++;
                break;
            }
            filled++;
            start_at++;
        }
        pa_block_unmap(pa_block, block_offset);
        if (is_end){
            node->file_pointer = start_at;
            return filled;
        }
        if (filled < n){
            index = fat[fd];
            block_offset = get_block_offset(index);
            start_at = 0;
        }
    }
    node->file_pointer = start_at;
    return filled;
}

int f_write(int fd, const char *str, int n){
    if (fd == STANDARD_OUT){
        int written = write(STDOUT_FILENO, str, n);
        return written;
    }
    if (!existed(fd)){
        error_code = 5;
        return -1;
    }
    file_descriptor *node = find_descriptor(fd);
    int filled = 0, index = fd, start = node->file_pointer;
    renew_time(fd);
    if (node->perm == F_READ){
        error_code = 3;
        return -1;
    }
    int block_offset = get_block_offset(fd);
    while (filled < n){
        int fill = add_data(str, block_offset, start, filled, n, n);
        if (fill == -1){
            return -1;
        }
        filled += fill;
        start += fill;
        if (str[filled] == '\0'){
            break;
        }
        if (filled < n){
            if (fat[index] == MAX_INDEX){
                int new_index = add_fat_entry();
                block_offset = get_block_offset(new_index);
                fat[index] = new_index;
            } else {
                block_offset = get_block_offset(fat[index]);
            }
            index = fat[index];
        }
    }
    int offset = locate_file_entry(node->file_name);
    int block_num = find_block_index(offset);
    int entry_index = find_entry_index(offset, block_num);
    int directory_offset = get_block_offset(block_num);
    int pa_offset = get_pa_offset(directory_offset);
    void *pa_block = pa_block_map(pa_offset);
    file_entry *directory = &pa_block[directory_offset - pa_offset];
    file_entry *entry = &directory[entry_index];
    entry->size += filled;
    node->file_pointer = start;
    remove_data(start + 1, index);
    remove_fat_entry(index);
    fat[index] = MAX_INDEX;
    pa_block_unmap(pa_block, pa_offset);
    return filled;
}

int f_close(int fd){
    if (fd == 0 || fd == 1){
        return 0;
    }
    if (!existed(fd)){
        error_code = 5;
        return -1;
    }
    renew_time(fd);
    return remove_fd(fd);
}

int f_unlink(const char *file_name){
    if (do_rm(file_name) < 0){
        return -1;
    }
    return 0;
}

int f_lseek(int fd, int offset, int whence){
    if (!existed(fd)){
        error_code = 5;
        return -1;
    }
    renew_time(fd);
    file_descriptor *node = find_descriptor(fd);
    switch(whence){
        case F_SEEK_CUR:{
            node->file_pointer += offset;
            break;
        }
        case F_SEEK_END:{
            int file_offset = locate_file_entry(node->file_name);
            int directory_block = find_block_index(file_offset);
            int file_entry_index = find_entry_index(file_offset, directory_block);
            int directory_block_offset = get_block_offset(directory_block);
            int directory_pa_offset = get_pa_offset(directory_block_offset);
            void *directory_pa_block = pa_block_map(directory_pa_offset);
            file_entry *file_directory = &directory_pa_block[directory_block_offset - directory_pa_offset];
            file_entry *cur_file_entry = &file_directory[file_entry_index];
            node->file_pointer = cur_file_entry->size + offset;
            pa_block_unmap(directory_pa_block, directory_pa_offset);
            break;
        }
        case F_SEEK_SET:{
            node->file_pointer = offset;
            break;
        }
        default:
            error_code = 8;
            return -1;
    }
    return 0;
}

void f_ls(const char *file_name){
    if (file_name == NULL){
        do_ls();
    } else {
        if (name_exist(file_name) < 0){
            error_code = 5;
            return;
        }
        fprintf(stderr, "%s\n", file_name);
    }
    
}

int f_move(char *src, char *dest) {
    if (do_mv(src, dest) == -1) {
        error_code = 8;
        return -1;
    }
    return 0;
}

void f_chmod(char *mode, char *file) {
    do_chmod(mode, file);
}

int f_mount(char *file_system) {
    if (do_mount(file_system) < 0){
        error_code = 8;
        return -1;
    }
    return 0;
}

int f_umount() {
    do_umount();
    return 0;
}

file_cmd *f_execute(char *file_name){
    int offset = locate_file_entry(file_name);
    if (offset < 0){
        error_code = 5;
        return NULL;
    }
    if (!can_execute_file(file_name)){
        error_code = 3;
        return NULL;
    }
    int block_size = get_block_size(get_config());
    int block_num = find_block_index(offset);
    int entry_index = find_entry_index(offset, block_num);
    int directory_offset = get_block_offset(block_num);
    int pa_entry_offset = get_pa_offset(directory_offset);
    void *pa_entry_block = pa_block_map(pa_entry_offset);
    file_entry *directory = &pa_entry_block[directory_offset - pa_entry_offset];
    file_entry *entry = &directory[entry_index];
    int size = entry->size;
    file_cmd *f_command = malloc(sizeof(file_cmd));
    f_command->argv = 0;
    int index = 0, local_pointer = 0, file_pointer = 0;
    int first_block = entry->first_block;
    char cmd_line[MAX_LIST];
    while (size > 0){
        int block_offset = get_block_offset(first_block);
        int pa_offset = get_pa_offset(block_offset);
        void * pa_block = pa_block_map(block_offset);
        char *block = &pa_block[block_offset - pa_offset];
        cmd_line[local_pointer] = block[file_pointer];
        local_pointer++; 
        file_pointer++;
        size--;
        if (block[file_pointer] == '\n'){
            char *line = copy_char(cmd_line, local_pointer);
            f_command->command_line[index] = line;
            f_command->argv++;
            local_pointer = 0;
            index++;
        }
        if (file_pointer == block_size){
            first_block = fat[first_block];
            file_pointer = 0;
        }
        pa_block_unmap(pa_block, block_offset);
    }   
    pa_block_unmap(pa_entry_block, pa_entry_offset);
    return f_command;
}