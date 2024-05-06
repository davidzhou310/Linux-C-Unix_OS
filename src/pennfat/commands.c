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
#include <math.h>

#include "helper.h"
#include "commands.h"
// #include "kernel.h"

int fs_fd;
uint16_t *fat;
int page_size;

//make file system
void do_mkfs(char *Args[]){
    page_size = sysconf(_SC_PAGE_SIZE);
    int fd = open(Args[1], O_RDWR | O_CREAT, 0644);
    if (fd == -1){
        perror("open error");
    }
    uint16_t blocksize_config = atoi(Args[3]);
    int block_size = get_block_size(blocksize_config);
    if (block_size == -1){
        perror("invalid blocksize config");
    }
    uint16_t num_blocks = atoi(Args[2]);
    if (num_blocks < 1 || num_blocks > 32) {
        perror("invalid number of blocks config");
    }
    int fat_size = block_size * num_blocks;
    int num_fat_entries = fat_size / 2;
    int data_size = block_size * (num_fat_entries - 1);
    int fs_size = fat_size + data_size;
    int ret = ftruncate(fd, fs_size);
    if (ret == -1) {
        perror("ftruncate");
    }
    fat = mmap(NULL, fat_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); //segmentation fault
    fat[0] = num_blocks << 8 | blocksize_config; //move blocks num 8 bytes to left and OR 16bytes config to get a 16bytes res
    fat[1] = MAX_INDEX; //assign 0*FFFF to fat[1];
    if (munmap(fat, fat_size) == -1){
        perror("munmap error");
    }
    close(fd);
}

//mount file system
int do_mount(char *file_name){
    fs_fd = open(file_name, O_RDWR);
    if (fs_fd == -1){
        perror("open");
        return -1;
    }
    int fat_size = get_fat_size();
    fat = mmap(NULL, fat_size, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    return 0;
}

//unmount file system
void do_umount(){
    int fat_size = get_fat_size();
    if (munmap(fat, fat_size) == -1){
        perror("munmap error");
    }
    close(fs_fd);
}

//create an empty file if doestn't exist, otherwise, renew its time
void do_touch(char *file_name){
    int offset = locate_file_entry(file_name);
    if (offset == -1){
        add_root_entry(file_name, 0, 0, REGULAR, RDWR);
    } else {
        int block_num = find_block_index(offset);
        int entry_index = find_entry_index(offset, block_num);
        int block_offset = get_block_offset(block_num);
        int pa_offset = get_pa_offset(block_offset);
        void * pa_block = pa_block_map(block_offset);
        file_entry *directory = &pa_block[block_offset - pa_offset];
        file_entry *entry = &directory[entry_index];
        entry->mtime = time(NULL);
        pa_block_unmap(pa_block, block_offset);
    }
}

//change file name
int do_mv(char *Args[]){
    int offset = locate_file_entry(Args[1]);
    if (offset == -1){
        return -1;
    }
    if (locate_file_entry(Args[2]) > 0){
        do_rm(Args[2]);
    }
    int block_num = find_block_index(offset);
    int entry_index = find_entry_index(offset, block_num);
    int block_offset = get_block_offset(block_num);
    int pa_offset = get_pa_offset(block_offset);
    void * pa_block = pa_block_map(block_offset);
    file_entry *directory = &pa_block[block_offset - pa_offset];
    file_entry *entry = &directory[entry_index];
    strcpy(entry->name, Args[2]);
    entry->mtime = time(NULL);
    pa_block_unmap(pa_block, block_offset);
    return 0;
}

//delete file
int do_rm(char *file_name){
    int offset = locate_file_entry(file_name);
    if (offset == -1){
        return -1;
    }
    int block_num = find_block_index(offset);
    int entry_index = find_entry_index(offset, block_num);
    int block_offset = get_block_offset(block_num);
    int pa_offset = get_pa_offset(block_offset);
    void * pa_block = pa_block_map(block_offset);
    file_entry *directory = &pa_block[block_offset - pa_offset];
    file_entry *entry = &directory[entry_index];
    entry->mtime = time(NULL);
    entry->name[0] = '1';
    if (entry->first_block != 0) { 
        remove_data(0, entry->first_block);
        remove_fat_entry(entry->first_block);
    }
    pa_block_unmap(pa_block, block_offset);
    return 0;
}

//overwrite file from stdin input, if file doesn't exist, create a new one
int do_cat_overwrite(ssize_t nread, char *inputBuffer, char *file_name){
    int filled = 0, n = nread;
    int offset = locate_file_entry(file_name);
    if (offset == -1) {
        int index = add_fat_entry();
        if (index == -1){
            perror("fat is full");
            return -1;
        } 
        add_root_entry(file_name, index, n, REGULAR, RDWR);
        while (n > 0){
            int block_offset = get_block_offset(index);
            int fill = add_data(inputBuffer, block_offset, 0, filled, n);
            if (fill == -1){
                return -1;
            }
            filled += fill;
            n -= fill;
            if (n > 0){
                int prev_index = index;
                index = add_fat_entry();
                if (index == -1){
                    perror("fat is full");
                    return -1;
                }
                fat[prev_index] = index;
            }
        }
    } else {
        int block_num = find_block_index(offset);
        int entry_index = find_entry_index(offset, block_num);
        int block_offset = get_block_offset(block_num);
        int pa_offset = get_pa_offset(block_offset);
        void * pa_block = pa_block_map(block_offset);
        file_entry *directory = &pa_block[block_offset - pa_offset];
        file_entry *entry = &directory[entry_index];
        int head_of_file = entry->first_block;
        if (head_of_file == 0) {
            int index = add_fat_entry();
            if (index == -1){
                perror("fat is full");
                return -1;
            } 
            head_of_file = index;
            entry->first_block = index;
        }
        int origin_size = entry->size;
        entry->size = n;
        entry->mtime = time(NULL);
        while (n > 0){
            int block_count = 0;
            int block_offset = get_block_offset(head_of_file);
            int fill = add_data(inputBuffer, block_offset, 0, filled, n);
            if (filled == -1){
                pa_block_unmap(pa_block, block_offset);
                return -1;
            }
            filled += fill;
            n -= fill;
            origin_size -= filled;
            if (n > 0 && origin_size <= 0){
                int prev_index = head_of_file;
                head_of_file = add_fat_entry();
                fat[prev_index] = head_of_file;
            } else if (n == 0 && origin_size > 0){
                remove_data(filled, head_of_file);
                if (fat[head_of_file] != MAX_INDEX){
                    remove_fat_entry(fat[head_of_file]);
                    fat[head_of_file] = MAX_INDEX;
                }
            }
            head_of_file = fat[head_of_file];
            block_count++;
        }
        pa_block_unmap(pa_block, block_offset);
    }
    return 0;
}

//append file from stdin input
int do_cat_append(ssize_t nread, char *inputBuffer, char *file_name){
    int filled = 0, n = nread;
    int offset = locate_file_entry(file_name);
    if (offset == -1){
        fprintf(stderr, "%s doesn't exist\n", file_name);
        return -1;
    }
    int block_num = find_block_index(offset);
    int entry_index = find_entry_index(offset, block_num);
    int block_size = get_block_size(get_config());
    int block_offset = get_block_offset(block_num);
    int pa_offset = get_pa_offset(block_offset);
    void * pa_block = pa_block_map(block_offset);
    file_entry *directory = &pa_block[block_offset - pa_offset];
    file_entry *entry = &directory[entry_index];

    // this need to call before update size
    int file_end_offset = locate_end_of_file(entry);
    // the file is empty
    if (file_end_offset == -1) {
        int index = add_fat_entry();
        if (index == -1){
            perror("fat is full");
            return -1;
        }
        entry->first_block = index;
    }

    int head_of_file = entry->first_block;
    entry->size += n;
    entry->mtime = time(NULL);
    int end_point = (file_end_offset - get_fat_size()) % block_size;
    // handle for the current block it already full
    if (n > 0 && end_point == 0) {
        int index = add_fat_entry();
        if (index == -1){
            perror("fat is full");
            return -1;
        }
        int prev_index = get_last_block(head_of_file);
        fat[prev_index] = index;
        file_end_offset = get_block_offset(index);
        end_point = 0;
    }
    while (n > 0){
        int fill = add_data(inputBuffer, file_end_offset - end_point, end_point, filled, n);
        if (fill == -1){
            pa_block_unmap(pa_block, block_offset);
            return -1;
        }
        filled += fill;
        n -= fill;
        if (n > 0){
            int prev_index = find_block_index(file_end_offset);
            int index = add_fat_entry();
            if (index == -1){
                perror("fat is full");
                pa_block_unmap(pa_block, block_offset);
                return -1;
            }
            fat[prev_index] = index;
            file_end_offset = get_block_offset(index);
            end_point = 0;
        }
    }
    pa_block_unmap(pa_block, block_offset);
    return 0;
}

//concat files and print it out on terminal
void do_cat(char *files[]){
    int index = 0;
    int block_size = get_block_size(get_config());
    while (files[index]){
        int offset = locate_file_entry(files[index]);
        if (offset == -1){
            fprintf(stderr, "%s doesn't exist\n", files[index]);
            return;
        }
        int block_num = find_block_index(offset);
        int entry_index = find_entry_index(offset, block_num);
        int block_offset = get_block_offset(block_num);
        int pa_offset = get_pa_offset(block_offset);
        void * pa_block = pa_block_map(block_offset);
        file_entry *directory = &pa_block[block_offset - pa_offset];
        file_entry *entry = &directory[entry_index];
        int head = entry->first_block;
        if (head == 0) {
            // file is empty
            pa_block_unmap(pa_block, block_offset);
            continue;
        }
        int file_size = entry->size;
        entry->mtime = time(NULL);  
        while (head != MAX_INDEX){
            int position = 0;
            int head_block_offset = get_block_offset(head);
            int head_pa_offset = get_pa_offset(head_block_offset);
            void * head_pa_block = pa_block_map(head_block_offset);
            char *file = &head_pa_block[head_block_offset - head_pa_offset];
            while (position < block_size && file_size > 0){
                fprintf(stderr, "%c", file[position]);
                file_size--;
                position++;
            }
            head = fat[head];
            pa_block_unmap(head_pa_block, head_block_offset);
        }
        pa_block_unmap(pa_block, block_offset);
        index++;
    }
}

//concat files and overwrite to the dest file, if file doesn't exist, create a new one
void do_concat_overwrite(char *files[], char *dest){
    int index = 0, dest_head, first_round = 1;
    file_entry *dest_entry, *dest_directory;
    int dest_offset = locate_file_entry(dest);
    while (files[index]){
        int cur_file_offset = locate_file_entry(files[index]);
        if (cur_file_offset == -1){
            fprintf(stderr, "%s doesn't exist\n", files[index]);
            return;
        }
        int directory_block = find_block_index(cur_file_offset);
        int file_entry_index = find_entry_index(cur_file_offset, directory_block);
        int directory_block_offset = get_block_offset(directory_block);
        int directory_pa_offset = get_pa_offset(directory_block_offset);
        void * directory_pa_block = pa_block_map(directory_pa_offset);
        file_entry *file_directory = &directory_pa_block[directory_block_offset - directory_pa_offset];
        file_entry *cur_file_entry = &file_directory[file_entry_index];
        cur_file_entry->mtime = time(NULL);
        if (dest_offset == -1){
            if (first_round) {
                dest_head = add_fat_entry();
                first_round = 0;
            }
            if (dest_head == -1){
                perror("fat is full");
                return;
            }
            add_root_entry(dest, dest_head, 0, REGULAR, RDWR);
            dest_offset = locate_file_entry(dest);
        }
        int dest_directory_block = find_block_index(dest_offset);
        int dest_entry_index = find_entry_index(dest_offset, dest_directory_block);
        int dest_directory_block_offset = get_block_offset(dest_directory_block);
        int dest_directory_pa_offset = get_pa_offset(dest_directory_block_offset);
        void * dest_directory_pa_block = pa_block_map(dest_directory_block_offset);
        dest_directory = &dest_directory_pa_block[dest_directory_block_offset - dest_directory_pa_offset];
        dest_entry = &dest_directory[dest_entry_index];
        if (first_round){
            if (dest_entry->first_block != 0) {
                remove_data(0, dest_entry->first_block);
            }
            first_round = 0;
        }
        copy_data(dest_entry, cur_file_entry);
        dest_entry->size += cur_file_entry->size;
        dest_entry->mtime = time(NULL);
        pa_block_unmap(dest_directory_pa_block, dest_directory_block_offset);
        pa_block_unmap(directory_pa_block, directory_block_offset);
        index++;
    }
}

//concat files and append to the dest file
void do_concat_append(char *files[], char *dest){
    int index = 0; 
    int dest_offset = locate_file_entry(dest);
    if (dest_offset == -1){
        fprintf(stderr, "%s doesn't exist\n", dest);
        return;
    }
    while (files[index]){
        int cur_file_offset = locate_file_entry(files[index]);
        if (cur_file_offset == -1){
            fprintf(stderr, "%s doesn't exist\n", files[index]);
            return;
        }
        int directory_block = find_block_index(cur_file_offset);
        int file_entry_index = find_entry_index(cur_file_offset, directory_block);
        int directory_block_offset = get_block_offset(directory_block);
        int directory_pa_offset = get_pa_offset(directory_block_offset);
        void * directory_pa_block = pa_block_map(directory_pa_offset);
        file_entry *file_directory = &directory_pa_block[directory_block_offset - directory_pa_offset];
        file_entry *cur_file_entry = &file_directory[file_entry_index];
        cur_file_entry->mtime= time(NULL);
        int dest_directory_block = find_block_index(dest_offset);
        int dest_entry_index = find_entry_index(dest_offset, dest_directory_block);
        int dest_block_offset = get_block_offset(dest_directory_block);
        int block_pa_offset = get_pa_offset(dest_block_offset);
        void *pa_block = pa_block_map(dest_block_offset);
        file_entry *dest_directory = &pa_block[dest_block_offset - block_pa_offset];
        file_entry *dest_entry = &dest_directory[dest_entry_index];
        copy_data(dest_entry, cur_file_entry);
        dest_entry->size += cur_file_entry->size;
        dest_entry->mtime = time(NULL);
        pa_block_unmap(directory_pa_block, directory_block_offset);
        pa_block_unmap(pa_block, dest_block_offset);
        index++;
    }
}

//list all files
void do_ls(){
    int directory_head = 1;
    while (directory_head != MAX_INDEX){
        int block_size = get_block_size(get_config());
        int num_file_entry = block_size / FILE_ENTRY_SIZE;
        int block_offset = get_block_offset(directory_head);
        int pa_offset = get_pa_offset(block_offset);
        void * cur_pa_block = pa_block_map(block_offset);
        file_entry *cur_block = &cur_pa_block[block_offset - pa_offset];
        for (int i = 0; i < num_file_entry; i++){
            char time_str[20];
            char str_perm[4]; 
            file_entry *entry = &cur_block[i];
            if (entry->name[0] == 0) {
                break;
            } else if (entry->name[0] == '1'){
                continue;
            }
            get_perm(entry->perm, str_perm);
            struct tm *tm_ptr = localtime(&entry->mtime);
            strftime(time_str, sizeof(time_str), " %b %d %H:%M", tm_ptr);
            fprintf(stderr, "%d %s %d %s %s\n", entry->first_block, str_perm, entry->size, time_str, entry->name);
            
        }
        pa_block_unmap(cur_pa_block, block_offset);
        directory_head = fat[directory_head];
    }
}

//copy file to a new file or overwrite the current file; or copy from host to file system
void do_cp(char *Args[], int argv){
    if (argv == 3){
        char *new_Args[2] = {Args[1], NULL};
        do_concat_overwrite(new_Args, Args[2]);
    } else {
        int first_round = 1;
        size_t len = 0;
        ssize_t nread;
        char *buffer = NULL;
        FILE *stream = fopen(Args[2], "r");
        if (stream == NULL){
            perror("fopen");
            return;
        }
        while ((nread = getline(&buffer, &len, stream)) != -1){
            if (first_round){
                if (do_cat_overwrite(nread, buffer, Args[3]) == -1){
                    return;
                }
                
            } else {
                if (do_cat_append(nread, buffer, Args[3]) == -1) {
                    return;
                }

            }
            first_round = 0;
        }
        free(buffer);
        fclose(stream);
    }
}

//copy file from file system to host
void do_cpto_host(char * source, char * dest) {
    int flag = O_RDWR | O_CREAT | O_TRUNC;
    int fd = open(dest, flag, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    int offset = locate_file_entry(source);
    if (offset == -1){
        fprintf(stderr, "%s doesn't exist\n", source);
        return;
    }
    int block_size = get_block_size(get_config());
    int block_num = find_block_index(offset);
    int entry_index = find_entry_index(offset, block_num);
    int block_offset = get_block_offset(block_num);
    int pa_offset = get_pa_offset(block_offset);
    void * pa_block = pa_block_map(block_offset);
    file_entry *directory = &pa_block[block_offset - pa_offset];
    file_entry *entry = &directory[entry_index];
    int head = entry->first_block;
    entry->mtime = time(NULL);
    if (head == 0) {
        pa_block_unmap(pa_block, block_offset);
        return;
    }
    int size = entry->size;
    while (head != MAX_INDEX){
        int head_block_offset = get_block_offset(head);
        int head_pa_offset = get_pa_offset(head_block_offset);
        void * head_pa_block = pa_block_map(head_block_offset);
        char *file = &head_pa_block[head_block_offset - head_pa_offset];
        
        // get number of char we need to read for each block
        int nwrite = size;
        size -= block_size;
        if (size >= 0) {
            nwrite = block_size;
        }
        // copy the block content to host file
        if (write(fd, file, nwrite) < 0) {
            perror("write");
            pa_block_unmap(head_pa_block, head_block_offset);
            pa_block_unmap(pa_block, block_offset);
            return;
        }
        head = fat[head];
        pa_block_unmap(head_pa_block, head_block_offset);
    }
    pa_block_unmap(pa_block, block_offset);
}

//change file permission
void do_chmod(char *mode, char *file) {
    // i.e. chmod +/- r/w/x FILE
    // x = 1; w = 2; r = 4
    int sign = 0;
    int update_mode = 0;
    switch(mode[0]) {
        case '+':
            sign = 1;
            break;
        case '-':
            sign = -1;
            break;
        default:
            break;
    }
    switch(mode[1]) {
        case 'x':
            update_mode = 1;
            break;
        case 'w':
            update_mode = 2;
            break;
        case 'r':
            update_mode = 4;
            break;
        default:
            break;
    }
    // get target file entry
    int offset = locate_file_entry(file);
    if (offset == -1){
        fprintf(stderr, "%s doesn't exist\n", file);
        return;
    }
    int block_num = find_block_index(offset);
    int entry_index = find_entry_index(offset, block_num);
    int block_offset = get_block_offset(block_num);
    int pa_offset = get_pa_offset(block_offset);
    void * pa_block = pa_block_map(block_offset);
    file_entry *directory = &pa_block[block_offset - pa_offset];
    file_entry *entry = &directory[entry_index];

    // update mode
    if (sign > 0) {
        entry->perm = entry->perm | update_mode;
    } else if (sign < 0) {
        entry->perm = entry->perm & (RWEX - update_mode);
    }

    pa_block_unmap(pa_block, block_offset);
}
