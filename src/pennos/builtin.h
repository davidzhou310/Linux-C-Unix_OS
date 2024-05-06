#include "process_list.h"

int built_in(struct parsed_command* parsed, int builtInFlag, process_node* head, process_node* tail);

int bg(process_node* head, process_node* tail, process_node* job);

int fg(process_node* head, process_node* tail, process_node* job);

void jobs_command(process_node* head, process_node* tail);