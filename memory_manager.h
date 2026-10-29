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

/* Public API for the Best Fit memory-allocation module. */
void init_memory(int total_size);
bool allocate_best_fit(Process process);
void print_memory_state(void);
void free_memory(int process_id);
/* Merge consecutive free blocks that are physically adjacent. */
void coalesce_memory(void);
void cleanup_memory(void);

#endif
