#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdbool.h>

typedef enum {
    ALGO_FIRST_FIT,
    ALGO_NEXT_FIT,
    ALGO_BEST_FIT,
    ALGO_WORST_FIT
} AlgorithmType;

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

typedef struct BenchmarkResult {
    AlgorithmType algo;
    const char* algo_name;
    int total_requests;
    int successful_allocs;
    int failed_allocs;
    int total_search_steps;
    double avg_search_steps;
    double final_mem_usage_pct;
    double final_ext_frag_pct;
    int max_hole_size;
} BenchmarkResult;

extern MemoryBlock* memory_head;
extern MemoryBlock* last_allocated_block;

void init_memory(int total_size);
void print_memory_state(void);
void render_htop_view(const char* current_event);
void free_memory(int process_id);
void coalesce_memory(void);
void cleanup_memory(void);
int get_total_memory_size(void);
void split_block(MemoryBlock* target, Process proc);

bool is_pid_active(int process_id);
bool allocate_first_fit(Process proc);
bool allocate_next_fit(Process proc);
bool allocate_best_fit(Process proc);
bool allocate_memory(Process proc, AlgorithmType algo);

/* ========================================================================= */
/* MODULE THỐNG KÊ & ĐÁNH GIÁ (EVALUATION MODULE) - THÀNH VIÊN 3             */
/* Được tách thành module độc lập: evaluation.h & evaluation.c               */
/* ========================================================================= */
#include "evaluation.h"

#endif
