#ifndef COMMANDS
#define COMMANDS

void do_mkfs(char *Args[]);

int do_mount(char *file_name);

void do_umount();

void do_touch(char *file_name);

int do_mv(char *Args[]);

int do_rm(char *file_name);

int do_cat_overwrite(ssize_t nread, char *inputBuffer, char *file_name);

int do_cat_append(ssize_t nread, char *inputBuffer, char *file_name);

void do_cat(char *files[]);

void do_concat_overwrite(char *files[], char *dest);

void do_concat_append(char *files[], char *dest);

void do_ls();

void do_cp(char *Args[], int argv);

void do_cpto_host(char *source, char *dest);

void do_chmod(char *mode, char *file);

#endif