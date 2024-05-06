#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "helper.h"

void trim_command(char *string){
    int a = 0, b = 0;
    while (string[a] != '\n'){
        if (isspace(string[a])){
            if (b > 0 && !isspace(string[b - 1])){
                string[b++] = ' ';
            }
        } else{
            string[b++] = string[a];
        }
        a++;
    }
    string[b] = '\0';
}

int get_block_size(int i){
    switch(i){
        case 0:
            return LSB_0;
        case 1:
            return LSB_1;
        case 2:
            return LSB_2;
        case 3:
            return LSB_3;
        case 4:
            return LSB_4;
        default:
            return -1;
    }
}

void get_perm(int perm, char str_perm[]){
    switch(perm){
        case 0:
            strcpy(str_perm, "---\0");
            break;
        case 2:
            strcpy(str_perm, "-w-\0");
            break;
        case 4:
            strcpy(str_perm, "r--\0");
            break;
        case 5:
            strcpy(str_perm, "r-x\0");
            break;
        case 6:
            strcpy(str_perm, "rw-\0");
            break;
        case 7:
            strcpy(str_perm, "rwx\0");
        default:
            return;
    }
}

int get_config(){
    uint8_t *temp_fat = mmap(NULL, FAT_ENTRY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    int config = temp_fat[0];
    if (munmap(temp_fat, FAT_ENTRY_SIZE) == -1){
        perror("munmap");
    }
    return config;
}

int get_num_block(){
    uint8_t *temp_fat = mmap(NULL, FAT_ENTRY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    int block_num = temp_fat[1];
    if (munmap(temp_fat, FAT_ENTRY_SIZE) == -1){
        perror("munmap");
    }
    return block_num;
}

int get_fat_size() {
    int num_of_block = get_num_block();
    int block_size = get_block_size(get_config());
    return num_of_block * block_size;
}

int find_block_index(int offset){
    int block_size = get_block_size(get_config());
    int block_num = (int)floor((offset - get_fat_size()) / block_size);
    return block_num + 1;
}

int find_entry_index(int offset, int block_num){
    int block_size = get_block_size(get_config());
    int directory_size = offset - get_fat_size() - (block_num - 1) * block_size;
    int index = directory_size / FILE_ENTRY_SIZE;
    return index;
}

int locate_file_entry(char *file_name){
    int block_size = get_block_size(get_config());
    int num_file_entry = block_size / FILE_ENTRY_SIZE;
    int cur_block_idx = 1;
    int block_offset = get_block_offset(cur_block_idx);
    while (cur_block_idx != MAX_INDEX) {
        int pa_offset = get_pa_offset(block_offset);
        void * cur_pa_block = pa_block_map(block_offset);
        file_entry *cur_block = &cur_pa_block[block_offset - pa_offset];
        for (int i = 0; i < num_file_entry; i++){
            file_entry *entry = &cur_block[i];
            if (entry->name[0] != 0 && strcmp(entry->name, file_name) == 0) {
                pa_block_unmap(cur_pa_block, block_offset);
                return i * FILE_ENTRY_SIZE + block_offset;
            }
            if (entry->name[0] == 0) {
                pa_block_unmap(cur_pa_block, block_offset);
                return -1;
            }
        }
        pa_block_unmap(cur_pa_block, block_offset);
        cur_block_idx = fat[cur_block_idx];
        block_offset = get_block_offset(cur_block_idx);
    }
    return -1;
}

int add_fat_entry(){
    int max_entry = get_fat_size() / FAT_ENTRY_SIZE;
    for (int index = 1; index < max_entry; ++index){
        if (fat[index] == 0){  
            fat[index] = MAX_INDEX;
            return index;
        }
    }
    return -1;
}

int locate_end_of_file(file_entry *entry){
    int num_blocks = 0;
    int head = entry->first_block;
    if (head == 0) {
        // file is empty
        return -1;
    }
    while (fat[head] != MAX_INDEX){
        num_blocks++;
        head = fat[head];
    }
    int block_size = get_block_size(get_config());
    int count = entry->size - block_size * num_blocks;
    int offset = get_block_offset(head);
    return offset + count;
}

int get_block_offset(int block_idx) {
    int block_size = get_block_size(get_config());
    return get_fat_size() + block_size * (block_idx - 1);
}

void add_root_entry(char *file_name, int index, int size, int type, int perm){
    int block_size = get_block_size(get_config());
    int num_file_entry = block_size / FILE_ENTRY_SIZE;
    int cur_block_idx = 1;
    int block_offset = get_block_offset(cur_block_idx);
    while (cur_block_idx != MAX_INDEX) {
        int pa_offset = get_pa_offset(block_offset);
        void * cur_pa_block = pa_block_map(block_offset);
        file_entry *cur_block = &cur_pa_block[block_offset - pa_offset];
        for (int i = 0; i < num_file_entry; i++){
            file_entry *entry = &cur_block[i];
            if (entry->name[0] == 0 || entry->name[0] == '1'){ //if empty or deleted, add directory
                strcpy(entry->name, file_name);
                entry->first_block = index;
                entry->mtime = time(NULL);
                entry->size = size;
                entry->type = type; 
                entry->perm = perm; 
                if (i ==  num_file_entry - 1) {
                    int new_block = add_fat_entry();
                    fat[cur_block_idx] = new_block;
                }
                pa_block_unmap(cur_pa_block, block_offset);
                return;
            }
        }
        pa_block_unmap(cur_pa_block, block_offset);
        cur_block_idx = fat[cur_block_idx];
        block_offset = get_block_offset(cur_block_idx);
    }
}

void remove_fat_entry(int index){
    while (fat[index] != MAX_INDEX){
        int next = fat[index];
        fat[index] = 0;
        index = next;
    }
    fat[index] = 0;
}

void remove_data(int filled, int head){
    int block_size = get_block_size(get_config());
    while (head != MAX_INDEX){
        int head_offset = get_block_offset(head);
        int head_pa_offset = get_pa_offset(head_offset);
        void * head_pa_block = pa_block_map(head_offset);
        char *block = &head_pa_block[head_offset - head_pa_offset];
        int index = filled;
        while (index < block_size){
            block[index] = 0;
            index++;
        }
        pa_block_unmap(head_pa_block, head_offset);
        head = fat[head];
        filled = 0;
    }
}

int add_data(char *inputBuffer, int block_offset, int file_from, int buffer_from, int remain){
    int r = remain;
    int block_size = get_block_size(get_config());
    int pa_offset = get_pa_offset(block_offset);
    void * pa_block = pa_block_map(block_offset);
    char *block = &pa_block[block_offset - pa_offset];
    int count = 0;
    while (file_from < block_size && r > 0){
        block[file_from] = inputBuffer[buffer_from];
        r--;
        buffer_from++;
        file_from++;
        count++;
    }
    pa_block_unmap(pa_block, block_offset);
    return count;
}

void copy_data(file_entry *dest_entry, file_entry *file_entry){
    int dest_head = dest_entry->first_block;
    int file_head = file_entry->first_block;
    if (file_head == 0) {
        // file is empty
        return;
    }
    if (dest_head == 0) {
        // add first block is destination file is empty
        int index = add_fat_entry();
        dest_entry->first_block = index;
        dest_head = index;
    }
    int file_size = file_entry->size;
    int block_size = get_block_size(get_config());
    int dest_offset = locate_end_of_file(dest_entry);
    int dest_index = (dest_offset - get_fat_size()) % block_size;
    dest_head = get_last_block(dest_head);
    if (dest_index == 0) {
        // handle the current last block is full
        dest_index = block_size;
    }
    int file_index = 0;
    while (file_head != MAX_INDEX){
        int file_head_offset = get_block_offset(file_head);
        int file_head_pa_offset = get_pa_offset(file_head_offset);
        void * file_head_pa_block = pa_block_map(file_head_offset);
        char *file_block = &file_head_pa_block[file_head_offset - file_head_pa_offset];

        int dest_head_offset = get_block_offset(dest_head);
        int dest_head_pa_offset = get_pa_offset(dest_head_offset);
        void * dest_head_pa_block = pa_block_map(dest_head_offset);
        char *dest_block = &dest_head_pa_block[dest_head_offset - dest_head_pa_offset];
        while (dest_index < block_size && file_index < block_size){
            dest_block[dest_index] = file_block[file_index];
            file_size--;
            if (file_size == 0){
                pa_block_unmap(file_head_pa_block, file_head_offset);
                pa_block_unmap(dest_head_pa_block, dest_head_offset);
                return;
            }
            dest_index++;
            file_index++;
        }
        pa_block_unmap(file_head_pa_block, file_head_offset);
        pa_block_unmap(dest_head_pa_block, dest_head_offset);
        if (dest_index == block_size && file_index < block_size){
            if (fat[dest_head] == MAX_INDEX){
                int next_block = add_fat_entry();
                fat[dest_head] = next_block;
                dest_head = next_block;
            } else {
                dest_head = fat[dest_head];
            }
            dest_index = 0;
            continue;
        } else if (dest_index < block_size && file_index == block_size){
            file_head = fat[file_head];
            file_index = 0;
            continue;
        }
        if (fat[dest_head] == MAX_INDEX){
            int next_block = add_fat_entry();
            fat[dest_head] = next_block;
            dest_head = next_block;
        } else {
            dest_head = fat[dest_head];
        }
        file_head = fat[file_head];
        dest_index = 0;
        file_index = 0;
    }
}

int get_pa_offset(int offset) {
    int pa_offset = offset & ~(SC_PAGE_SIZE - 1);
    return pa_offset;
}

void * pa_block_map(int offset) {
    int pa_offset = get_pa_offset(offset);
    int len = get_block_size(get_config()) + offset - pa_offset;
    void *pa_block = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, pa_offset);
    return pa_block;
}

void pa_block_unmap(void * pa_block, int offset) {
    int pa_offset = get_pa_offset(offset);
    int len = get_block_size(get_config()) + offset - pa_offset;
    if (munmap(pa_block, len) == -1){
        perror("munmap");
    }
}

int get_file_perm(char *file_name) {
    int offset = locate_file_entry(file_name);
    // file not exist
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
    int perm = entry->perm;
    pa_block_unmap(pa_block, block_offset);
    return perm;
}

int can_read_file(char *file_name) {
    int perm = get_file_perm(file_name);
    // file not exist
    if (perm == -1) {
        return -1;
    }
    
    int can_read = perm & RDON;
    if (can_read > 0) {
        return 1;
    }
    return 0;
}

int can_read_files(char *files[]) {
    int index = 0;
    while(files[index]) {
        if (can_read_file(files[index]) == 0) {
            return 0;
        }
        index++;
    }
    return 1;
}

int can_write_file(char *file_name) {
    int perm = get_file_perm(file_name);
    // file not exist
    if (perm == -1) {
        return -1;
    }
    
    int can_read = perm & WRON;
    if (can_read > 0) {
        return 1;
    }
    return 0;
}

int get_last_block(int first_block) {
    int index = first_block;
    while (fat[index] != MAX_INDEX){
        index = fat[index];
    }
    return index;
}
