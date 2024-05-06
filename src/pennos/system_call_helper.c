#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "system_call_helper.h"
#include "commands_helper.h"
#include "kernel.h"

file_descriptor *head = NULL;

file_descriptor *new_fd(char *file_name, int perm, int fd, int file_pointer){
    file_descriptor *node;
    node = malloc(sizeof(file_descriptor));
    node->file_name = file_name;
    node->perm = perm;
    node->descriptor = fd;
    node->file_pointer = file_pointer;
    node->next = NULL;
    return node;
}

int have_f_write(){
    list_node* head = qs.all_process->next;
    while (head != NULL) {
        file_descriptor *cur_node = head->pcb->file_descriptor;
        while (cur_node){
            if (cur_node->perm == F_WRITE){
                return 1;
            }
            cur_node = cur_node->next;
        }
        head = head->next;
    }
    return 0;
}

int existed(int fd){
    if (fd == STANDARD_IN || fd == STANDARD_OUT){
        return 0;
    }
    file_descriptor *cur_node = head;
    while (cur_node){
        if (cur_node->descriptor == fd){
            return 1;
        }
        cur_node = cur_node->next;
    }
    return 0;
}

int name_exist(char *name){
    file_descriptor *cur_node = head;
    while (cur_node){
        if (strcmp(cur_node->file_name, name) == 0){
            return cur_node->descriptor;
        }
        cur_node = cur_node->next;
    }
    return -1;
}

void renew_time(int fd){
    file_descriptor *node = find_descriptor(fd);
    int offset = locate_file_entry(node->file_name);
    int block_num = find_block_index(offset);
    int entry_index = find_entry_index(offset, block_num);
    int block_offset = get_block_offset(block_num);
    int pa_offset = get_pa_offset(block_offset);
    void *pa_block = pa_block_map(pa_offset);
    file_entry *directory = &pa_block[block_offset - pa_offset];
    file_entry *entry = &directory[entry_index];
    entry->mtime = time(NULL);
    pa_block_unmap(pa_block, block_offset);
}

file_descriptor *find_descriptor(int fd){
    file_descriptor *cur_node = head;
    while (cur_node){
        if (cur_node->descriptor == fd){
            return cur_node;
        }
        cur_node = cur_node->next;
    }
    return NULL;
}

void add_fd(char *file_name, int perm, int fd, int file_pointer){
    if (existed(fd)){
        file_descriptor *cur_node = head;
        while (cur_node){
            if (cur_node->descriptor == fd){
                cur_node->perm = perm;
                return;
            }
            cur_node = cur_node->next;
        }
    }
    file_descriptor *cur_node = head;
    file_descriptor *new_node = new_fd(file_name, perm, fd, file_pointer);
    if (!head){
        head = new_node;
        return;
    }
    while (cur_node->next){
        if (!cur_node->next){
            break;
        }
        cur_node = cur_node->next;
    }
    cur_node->next = new_node;  
}

int remove_fd(int fd){
    file_descriptor *cur_node = head;
    if (head->descriptor == fd){
        file_descriptor *temp = head;
        head = head->next;
        free(temp);
        return 0;
    }
    while(cur_node->next){
        if (cur_node->next->descriptor == fd){
            file_descriptor *temp = cur_node->next;
            cur_node->next = cur_node->next->next;
            free(temp);
            return 0;
        }
        cur_node = cur_node->next;
    }
    return -1;
}

char *copy_char(char *target_list, int size){
    char *buffer = (char *)malloc(size * sizeof(char));
    for (int i = size - 1; i > -1; i--){
        buffer[i] = target_list[i];
    }
    buffer[size] = '\0';
    return buffer;
}