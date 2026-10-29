#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdbool.h>

typedef struct Process {
    int process_id;
    int size_needed;
} Process;

typedef struct MemoryBlock {
    int start_address;
    int size;
    bool is_allocated;
    int process_id;
    struct MemoryBlock* next;
} MemoryBlock;

extern MemoryBlock* memory_head;
extern MemoryBlock* last_allocated_block;

void init_memory(int total_size);
void print_memory_state(void);
void free_memory(int process_id);
void cleanup_memory(void);

bool allocate_next_fit(Process proc);

#endif
