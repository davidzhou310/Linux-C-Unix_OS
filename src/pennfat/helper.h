#ifndef HELPER
#define HELPER

#include <stdint.h>

#define LSB_0 256
#define LSB_1 512
#define LSB_2 1024
#define LSB_3 2048
#define LSB_4 4096

#define MAX_INPUT 1024
#define MAX_SIZE 100000
#define MAX_LIST 100
#define FAT_ENTRY_SIZE 2
#define MAX_INDEX 65535
#define FILE_ENTRY_SIZE 64

#define NONE 0
#define WRON 2
#define RDON 4
#define RDEX 5
#define RDWR 6
#define RWEX 7

#define UNKNOWN 0
#define REGULAR 1
#define DERICTORY 2
#define SYMBOLIC 4

#define SC_PAGE_SIZE sysconf(_SC_PAGE_SIZE)

typedef struct file_entry file_entry;

struct file_entry {
    char name[32];
    uint32_t size;
    uint16_t first_block;
    uint8_t type;
    uint8_t perm;
    time_t mtime;
    char remaining_space[16];
};

extern int fs_fd;
extern uint16_t *fat;

void trim_command(char *string);

int get_block_size(int i);

int get_config();

int get_num_block();

int get_fat_size();

void get_perm(int perm, char str_perm[]);

int locate_file_entry(char *file_name);

int find_block_index(int offset);

int find_entry_index(int offset, int block_num);

int add_fat_entry();

void remove_fat_entry(int index);

void add_root_entry(char *file_name, int index, int size, int type, int perm);

int get_block_offset(int block_idx);

int locate_end_of_file(file_entry *entry);

void remove_data(int filled, int head);

int add_data(char *inputBuffer, int block_offset, int file_from, int buffer_from, int remain);

void copy_data(file_entry *dest_entry, file_entry *file_entry);

int get_pa_offset(int offset);

void * pa_block_map(int offset);

void pa_block_unmap(void * pa_block, int offset);

int get_file_perm(char *file_name);

int can_read_file(char *file_name);

int can_read_files(char *files[]);

int can_write_file(char *file_name);

int get_last_block(int first_block);

#endif