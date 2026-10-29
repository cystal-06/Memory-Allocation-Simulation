#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "memory_manager.h"

MemoryBlock* memory_head = NULL;
MemoryBlock* last_allocated_block = NULL;

static int g_total_memory_size = 0;
static int g_total_alloc_requests = 0;
static int g_successful_allocs = 0;
static int g_total_search_steps = 0;
static BenchmarkResult g_last_benchmark_results[4];
static bool g_has_benchmark_results = false;

void init_memory(int total_size) {
    if (total_size <= 0) {
        printf("\033[1;31m[CRITICAL] Memory pool size must be greater than zero.\033[0m\n");
        return;
    }

    if (memory_head != NULL) {
        cleanup_memory();
    }

    MemoryBlock* initial_block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    if (initial_block == NULL) {
        printf("\033[1;31m[KERNEL PANIC] Out of heap memory allocating partition node.\033[0m\n");
        exit(EXIT_FAILURE);
    }

    initial_block->start_address = 0;
    initial_block->size = total_size;
    initial_block->is_allocated = false;
    initial_block->process_id = -1;
    initial_block->next = NULL;

    memory_head = initial_block;
    last_allocated_block = initial_block;
    g_total_memory_size = total_size;
    g_total_alloc_requests = 0;
    g_successful_allocs = 0;
    g_total_search_steps = 0;
}

bool is_pid_active(int process_id) {
    MemoryBlock* curr = memory_head;
    while (curr != NULL) {
        if (curr->is_allocated && curr->process_id == process_id) {
            return true;
        }
        curr = curr->next;
    }
    return false;
}

static void print_gauge(const char* label, double percent, int width, const char* unit_str) {
    printf("\033[1;36m%-4s\033[0m[", label);
    int filled = (int)((percent / 100.0) * width);
    if (filled > width) filled = width;
    if (filled < 0) filled = 0;

    for (int i = 0; i < width; i++) {
        if (i < filled) {
            if (i < width * 0.50) {
                printf("\033[1;32m|\033[0m");
            } else if (i < width * 0.80) {
                printf("\033[1;33m|\033[0m");
            } else {
                printf("\033[1;31m|\033[0m");
            }
        } else {
            printf("\033[1;30m.\033[0m");
        }
    }
    printf("] \033[1;37m%5.1f%%\033[0m %s", percent, unit_str ? unit_str : "");
}

void render_htop_view(const char* current_event) {
    if (memory_head == NULL) {
        return;
    }

    int total_allocated = 0;
    int total_free = 0;
    int total_blocks = 0;
    int active_procs = 0;
    int max_free_hole = 0;

    MemoryBlock* curr = memory_head;
    while (curr != NULL) {
        total_blocks++;
        if (curr->is_allocated) {
            total_allocated += curr->size;
            active_procs++;
        } else {
            total_free += curr->size;
            if (curr->size > max_free_hole) {
                max_free_hole = curr->size;
            }
        }
        curr = curr->next;
    }

    double mem_usage_pct = g_total_memory_size > 0 ? (total_allocated * 100.0 / g_total_memory_size) : 0.0;
    double ext_frag_pct = (total_free > 0) ? (1.0 - (double)max_free_hole / total_free) * 100.0 : 0.0;
    double success_rate = (g_total_alloc_requests > 0) ? (g_successful_allocs * 100.0 / g_total_alloc_requests) : 100.0;
    double avg_steps = (g_successful_allocs > 0) ? ((double)g_total_search_steps / g_successful_allocs) : 1.0;

    printf("\033[H\033[J");

    printf("\033[1;37;44m  HTOP / SYSTEM KERNEL MONITOR - DYNAMIC MEMORY ALLOCATOR [NEXT-FIT]      \033[0m\n");

    print_gauge("0", (mem_usage_pct * 0.75 + 12.0 > 100.0 ? 94.2 : mem_usage_pct * 0.75 + 12.0), 20, " ");
    printf("   \033[1;36mTasks:\033[0m \033[1;37m%d\033[0m total, \033[1;32m%d active\033[0m, \033[1;34m%d holes\033[0m\n",
           total_blocks, active_procs, total_blocks - active_procs);

    print_gauge("1", (mem_usage_pct * 0.55 + 8.5 > 100.0 ? 82.1 : mem_usage_pct * 0.55 + 8.5), 20, " ");
    printf("   \033[1;36mLoad avg:\033[0m \033[1;32m0.14\033[0m \033[1;33m0.28\033[0m \033[1;31m0.42\033[0m | Uptime: \033[1;37m00:04:18\033[0m\n");

    char mem_detail[32];
    snprintf(mem_detail, sizeof(mem_detail), "\033[1;37m%d\033[0m/\033[1;32m%dB\033[0m", total_allocated, g_total_memory_size);
    print_gauge("Mem", mem_usage_pct, 20, mem_detail);
    printf("   \033[1;36mFragmentation (EFR):\033[0m \033[1;33m%.1f%%\033[0m (MaxHole: %dB)\n", ext_frag_pct, max_free_hole);

    int ptr_addr = last_allocated_block ? last_allocated_block->start_address : 0;
    int ptr_pid = last_allocated_block ? last_allocated_block->process_id : -1;
    char swp_detail[32];
    snprintf(swp_detail, sizeof(swp_detail), "\033[1;37m0\033[0m/\033[1;32m512B\033[0m ");
    print_gauge("Swp", 0.0, 20, swp_detail);
    printf("   \033[1;36mPointer Reg:\033[0m \033[1;35mPTR_LAST_ALLOC = 0x%04X (PID %d)\033[0m\n", ptr_addr, ptr_pid);

    printf("\n\033[1;37;40m ENGINE TELEMETRY \033[0m  Strategy: \033[1;32mKnuth Next-Fit (Circular)\033[0m | Avg Search: \033[1;33m%.2f steps\033[0m | Success: \033[1;32m%.1f%%\033[0m\n",
           avg_steps, success_rate);

    printf("\n\033[1;37m[ RAM PHYSICAL SEGMENT LAYOUT MAP ]\033[0m\n");
    printf("+");
    for (int i = 0; i < 76; i++) printf("-");
    printf("+\n|");

    int bar_width = 76;
    curr = memory_head;
    int chars_printed = 0;
    while (curr != NULL) {
        int w = (curr->size * bar_width) / g_total_memory_size;
        if (w < 1) w = 1;
        if (chars_printed + w > bar_width || curr->next == NULL) {
            w = bar_width - chars_printed;
        }

        const char* color = "\033[47;30m";
        if (curr == last_allocated_block && curr->is_allocated) {
            color = "\033[43;30m";
        } else if (curr->is_allocated) {
            int pid_color = curr->process_id % 6;
            switch (pid_color) {
                case 1: color = "\033[42;30m"; break;
                case 2: color = "\033[44;37m"; break;
                case 3: color = "\033[45;37m"; break;
                case 4: color = "\033[46;30m"; break;
                default: color = "\033[41;37m"; break;
            }
        } else {
            color = (curr == last_allocated_block) ? "\033[103;30m" : "\033[100;37m";
        }

        char label[32];
        if (curr->is_allocated) {
            snprintf(label, sizeof(label), "P%d:%dB", curr->process_id, curr->size);
        } else {
            snprintf(label, sizeof(label), "FREE:%dB", curr->size);
        }

        int label_len = strlen(label);
        printf("%s", color);
        for (int k = 0; k < w; k++) {
            if (k < label_len && label_len <= w) {
                putchar(label[k]);
            } else {
                putchar(' ');
            }
        }
        printf("\033[0m");

        chars_printed += w;
        curr = curr->next;
    }
    printf("|\n+");
    for (int i = 0; i < 76; i++) printf("-");
    printf("+\n");

    printf("\033[1;30m0x0000        0x00FA        0x01F4        0x02EE        0x03E8 (1000B)\033[0m\n\n");

    printf("\033[46;30m PID   OWNER    START_ADDR   END_ADDR     SIZE      STATE        EFR_RATIO  POINTER_LOC     \033[0m\n");

    curr = memory_head;
    while (curr != NULL) {
        int end_addr = curr->start_address + curr->size - 1;
        double block_pct = (curr->size * 100.0) / g_total_memory_size;
        bool is_pointer = (curr == last_allocated_block);

        if (is_pointer) {
            printf("\033[1;33m");
        }

        if (curr->is_allocated) {
            printf(" %-5d \033[1;32m%-8s\033[0m%s 0x%04X       0x%04X       %-7dB   \033[1;32m%-10s\033[0m%s  %5.1f%%     %s\n",
                   curr->process_id,
                   "sys_proc",
                   is_pointer ? "\033[1;33m" : "",
                   curr->start_address,
                   end_addr,
                   curr->size,
                   "ALLOCATED",
                   is_pointer ? "\033[1;33m" : "",
                   block_pct,
                   is_pointer ? "<== [ACTIVE POINTER]" : "");
        } else {
            printf(" %-5s \033[1;34m%-8s\033[0m%s 0x%04X       0x%04X       %-7dB   \033[1;34m%-10s\033[0m%s  %5.1f%%     %s\n",
                   "FREE",
                   "unmapped",
                   is_pointer ? "\033[1;33m" : "",
                   curr->start_address,
                   end_addr,
                   curr->size,
                   "IDLE_HOLE",
                   is_pointer ? "\033[1;33m" : "",
                   block_pct,
                   is_pointer ? "<== [ACTIVE POINTER]" : "");
        }

        if (is_pointer) {
            printf("\033[0m");
        }

        curr = curr->next;
    }

    printf("\n\033[1;30;47m SYSTEM EVENT BUS \033[0m \033[1;37m%s\033[0m\n", current_event ? current_event : "System idle.");
    fflush(stdout);
}

void print_memory_state(void) {
    render_htop_view("Telemetry snapshot captured.");
}

static void split_block(MemoryBlock* target, Process proc) {
    if (target == NULL || target->is_allocated || target->size < proc.size_needed) {
        return;
    }

    if (target->size == proc.size_needed) {
        target->is_allocated = true;
        target->process_id = proc.process_id;
        return;
    }

    MemoryBlock* remainder_block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    if (remainder_block == NULL) {
        exit(EXIT_FAILURE);
    }

    remainder_block->start_address = target->start_address + proc.size_needed;
    remainder_block->size = target->size - proc.size_needed;
    remainder_block->is_allocated = false;
    remainder_block->process_id = -1;
    remainder_block->next = target->next;

    target->size = proc.size_needed;
    target->is_allocated = true;
    target->process_id = proc.process_id;
    target->next = remainder_block;
}

bool allocate_first_fit(Process proc) {
    g_total_alloc_requests++;
    if (memory_head == NULL || proc.process_id <= 0 || proc.size_needed <= 0 || is_pid_active(proc.process_id)) {
        return false;
    }

    MemoryBlock* curr = memory_head;
    int steps = 0;
    while (curr != NULL) {
        steps++;
        if (!curr->is_allocated && curr->size >= proc.size_needed) {
            split_block(curr, proc);
            g_successful_allocs++;
            g_total_search_steps += steps;
            return true;
        }
        curr = curr->next;
    }

    g_total_search_steps += steps;
    return false;
}

bool allocate_next_fit(Process proc) {
    g_total_alloc_requests++;

    if (memory_head == NULL || proc.process_id <= 0 || proc.size_needed <= 0 || is_pid_active(proc.process_id)) {
        return false;
    }

    if (last_allocated_block == NULL) {
        last_allocated_block = memory_head;
    }

    MemoryBlock* start = last_allocated_block;
    MemoryBlock* current = start;
    bool wrapped_around = false;
    int steps = 0;

    while (current != NULL) {
        steps++;
        if (!current->is_allocated && current->size >= proc.size_needed) {
            split_block(current, proc);
            last_allocated_block = current;
            g_successful_allocs++;
            g_total_search_steps += steps;
            return true;
        }

        current = current->next;

        if (current == NULL && !wrapped_around) {
            current = memory_head;
            wrapped_around = true;
        }

        if (wrapped_around && current == start) {
            break;
        }
    }

    g_total_search_steps += steps;
    return false;
}

bool allocate_best_fit(Process proc) {
    g_total_alloc_requests++;
    if (memory_head == NULL || proc.process_id <= 0 || proc.size_needed <= 0 || is_pid_active(proc.process_id)) {
        return false;
    }

    MemoryBlock* curr = memory_head;
    MemoryBlock* best_block = NULL;
    int min_diff = 1000000000;
    int steps = 0;

    while (curr != NULL) {
        steps++;
        if (!curr->is_allocated && curr->size >= proc.size_needed) {
            int diff = curr->size - proc.size_needed;
            if (diff < min_diff) {
                min_diff = diff;
                best_block = curr;
                if (diff == 0) {
                    break;
                }
            }
        }
        curr = curr->next;
    }

    g_total_search_steps += steps;
    if (best_block != NULL) {
        split_block(best_block, proc);
        g_successful_allocs++;
        return true;
    }

    return false;
}

bool allocate_worst_fit(Process proc) {
    g_total_alloc_requests++;
    if (memory_head == NULL || proc.process_id <= 0 || proc.size_needed <= 0 || is_pid_active(proc.process_id)) {
        return false;
    }

    MemoryBlock* curr = memory_head;
    MemoryBlock* worst_block = NULL;
    int max_size = -1;
    int steps = 0;

    while (curr != NULL) {
        steps++;
        if (!curr->is_allocated && curr->size >= proc.size_needed) {
            if (curr->size > max_size) {
                max_size = curr->size;
                worst_block = curr;
            }
        }
        curr = curr->next;
    }

    g_total_search_steps += steps;
    if (worst_block != NULL) {
        split_block(worst_block, proc);
        g_successful_allocs++;
        return true;
    }

    return false;
}

bool allocate_memory(Process proc, AlgorithmType algo) {
    switch (algo) {
        case ALGO_FIRST_FIT: return allocate_first_fit(proc);
        case ALGO_NEXT_FIT:  return allocate_next_fit(proc);
        case ALGO_BEST_FIT:  return allocate_best_fit(proc);
        case ALGO_WORST_FIT: return allocate_worst_fit(proc);
        default: return false;
    }
}

void coalesce_memory(void) {
    if (memory_head == NULL) {
        return;
    }

    MemoryBlock* current = memory_head;
    while (current != NULL && current->next != NULL) {
        if (!current->is_allocated && !current->next->is_allocated) {
            MemoryBlock* temp = current->next;
            current->size += temp->size;
            current->next = temp->next;
            if (last_allocated_block == temp) {
                last_allocated_block = current;
            }
            free(temp);
        } else {
            current = current->next;
        }
    }
}

void free_memory(int process_id) {
    if (memory_head == NULL) {
        return;
    }

    MemoryBlock* current = memory_head;
    while (current != NULL) {
        if (current->is_allocated && current->process_id == process_id) {
            current->is_allocated = false;
            current->process_id = -1;
            if (last_allocated_block == current) {
                last_allocated_block = memory_head;
            }
        }
        current = current->next;
    }

    coalesce_memory();
}

void cleanup_memory(void) {
    MemoryBlock* current = memory_head;
    while (current != NULL) {
        MemoryBlock* next_node = current->next;
        free(current);
        current = next_node;
    }
    memory_head = NULL;
    last_allocated_block = NULL;
    g_total_memory_size = 0;
    g_total_alloc_requests = 0;
    g_successful_allocs = 0;
    g_total_search_steps = 0;
}

static BenchmarkResult evaluate_single_algo(AlgorithmType algo, const char* name, int ram_size) {
    init_memory(ram_size);

    Process test_suite[] = {
        {.process_id = 1, .size_needed = 120},
        {.process_id = 2, .size_needed = 180},
        {.process_id = 3, .size_needed = 90},
        {.process_id = 4, .size_needed = 220},
        {.process_id = 5, .size_needed = 150},
        {.process_id = 6, .size_needed = 80},
        {.process_id = 7, .size_needed = 110},
        {.process_id = 8, .size_needed = 70}
    };
    int total_procs = sizeof(test_suite) / sizeof(test_suite[0]);

    for (int i = 0; i < total_procs; i++) {
        allocate_memory(test_suite[i], algo);
    }

    free_memory(2);
    free_memory(4);
    free_memory(6);

    Process late_arrivals[] = {
        {.process_id = 9,  .size_needed = 100},
        {.process_id = 10, .size_needed = 130},
        {.process_id = 11, .size_needed = 160},
        {.process_id = 12, .size_needed = 75}
    };
    int late_count = sizeof(late_arrivals) / sizeof(late_arrivals[0]);

    for (int i = 0; i < late_count; i++) {
        allocate_memory(late_arrivals[i], algo);
    }

    int total_allocated = 0;
    int total_free = 0;
    int max_hole = 0;

    MemoryBlock* curr = memory_head;
    while (curr != NULL) {
        if (curr->is_allocated) {
            total_allocated += curr->size;
        } else {
            total_free += curr->size;
            if (curr->size > max_hole) {
                max_hole = curr->size;
            }
        }
        curr = curr->next;
    }

    BenchmarkResult res;
    res.algo = algo;
    res.algo_name = name;
    res.total_requests = g_total_alloc_requests;
    res.successful_allocs = g_successful_allocs;
    res.failed_allocs = g_total_alloc_requests - g_successful_allocs;
    res.total_search_steps = g_total_search_steps;
    res.avg_search_steps = (g_successful_allocs > 0) ? ((double)g_total_search_steps / g_successful_allocs) : 0.0;
    res.final_mem_usage_pct = (ram_size > 0) ? (total_allocated * 100.0 / ram_size) : 0.0;
    res.final_ext_frag_pct = (total_free > 0) ? (1.0 - (double)max_hole / total_free) * 100.0 : 0.0;
    res.max_hole_size = max_hole;

    cleanup_memory();
    return res;
}

void export_benchmark_csv(const char* filepath) {
    if (!g_has_benchmark_results) {
        return;
    }

    FILE* f = fopen(filepath, "w");
    if (f == NULL) {
        printf("\033[1;31m[ERROR] Unable to write benchmark CSV to %s\033[0m\n", filepath);
        return;
    }

    fprintf(f, "Algorithm,Total_Requests,Successful_Allocs,Failed_Allocs,Success_Rate_Pct,Avg_Search_Steps,Final_Mem_Usage_Pct,Final_Ext_Frag_Pct,Max_Hole_Bytes\n");
    for (int i = 0; i < 4; i++) {
        double s_rate = (g_last_benchmark_results[i].successful_allocs * 100.0) / g_last_benchmark_results[i].total_requests;
        fprintf(f, "%s,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%d\n",
                g_last_benchmark_results[i].algo_name,
                g_last_benchmark_results[i].total_requests,
                g_last_benchmark_results[i].successful_allocs,
                g_last_benchmark_results[i].failed_allocs,
                s_rate,
                g_last_benchmark_results[i].avg_search_steps,
                g_last_benchmark_results[i].final_mem_usage_pct,
                g_last_benchmark_results[i].final_ext_frag_pct,
                g_last_benchmark_results[i].max_hole_size);
    }
    fclose(f);
    printf("\033[1;32m[EXPORT SUCCESS] Benchmark telemetry saved to %s\033[0m\n", filepath);
}

void run_comparison_benchmark(int total_size) {
    g_last_benchmark_results[0] = evaluate_single_algo(ALGO_FIRST_FIT, "First Fit", total_size);
    g_last_benchmark_results[1] = evaluate_single_algo(ALGO_NEXT_FIT,  "Next Fit",  total_size);
    g_last_benchmark_results[2] = evaluate_single_algo(ALGO_BEST_FIT,  "Best Fit",  total_size);
    g_last_benchmark_results[3] = evaluate_single_algo(ALGO_WORST_FIT, "Worst Fit", total_size);
    g_has_benchmark_results = true;

    printf("\n\033[1;37;44m ========================================================================================= \033[0m\n");
    printf("\033[1;37;44m  SCIENTIFIC BENCHMARK REPORT: ALLOCATION STRATEGY PERFORMANCE EVALUATION                 \033[0m\n");
    printf("\033[1;37;44m ========================================================================================= \033[0m\n");
    printf(" Test Workload: 12 Mixed Processes (Alloc & Free holes) | Initial RAM: \033[1;32m%d Bytes\033[0m\n\n", total_size);

    printf("\033[46;30m ALGORITHM   TOTAL REQ   SUCCESS   FAILED   SUCCESS_RATE   AVG_STEPS   MEM_USAGE%%   EFR_FRAG%% \033[0m\n");

    for (int i = 0; i < 4; i++) {
        double s_rate = (g_last_benchmark_results[i].successful_allocs * 100.0) / g_last_benchmark_results[i].total_requests;
        printf(" \033[1;37m%-11s\033[0m %-11d \033[1;32m%-9d\033[0m \033[1;31m%-8d\033[0m \033[1;33m%11.1f%%\033[0m   \033[1;36m%9.2f\033[0m   \033[1;32m%9.1f%%\033[0m   \033[1;35m%8.1f%%\033[0m\n",
               g_last_benchmark_results[i].algo_name,
               g_last_benchmark_results[i].total_requests,
               g_last_benchmark_results[i].successful_allocs,
               g_last_benchmark_results[i].failed_allocs,
               s_rate,
               g_last_benchmark_results[i].avg_search_steps,
               g_last_benchmark_results[i].final_mem_usage_pct,
               g_last_benchmark_results[i].final_ext_frag_pct);
    }

    printf("+-----------------------------------------------------------------------------------------+\n");
    printf("\033[1;33m [ASCII VISUAL COMPARISON CHARTS]\033[0m\n\n");

    printf("  \033[1;36mSearch Overhead (Avg Steps - Lower is faster):\033[0m\n");
    for (int i = 0; i < 4; i++) {
        int bar_len = (int)(g_last_benchmark_results[i].avg_search_steps * 3.5);
        if (bar_len < 1) bar_len = 1;
        printf("  %-10s [", g_last_benchmark_results[i].algo_name);
        for (int k = 0; k < 30; k++) {
            if (k < bar_len) printf("\033[1;32m#\033[0m");
            else printf("\033[1;30m.\033[0m");
        }
        printf("] \033[1;37m%.2f steps\033[0m\n", g_last_benchmark_results[i].avg_search_steps);
    }

    printf("\n  \033[1;36mExternal Fragmentation Ratio (EFR - Lower is less fragmented):\033[0m\n");
    for (int i = 0; i < 4; i++) {
        int bar_len = (int)(g_last_benchmark_results[i].final_ext_frag_pct * 0.30);
        if (bar_len < 1) bar_len = 1;
        printf("  %-10s [", g_last_benchmark_results[i].algo_name);
        for (int k = 0; k < 30; k++) {
            if (k < bar_len) printf("\033[1;35m#\033[0m");
            else printf("\033[1;30m.\033[0m");
        }
        printf("] \033[1;37m%.1f%%\033[0m\n", g_last_benchmark_results[i].final_ext_frag_pct);
    }

    printf("+-----------------------------------------------------------------------------------------+\n");
    export_benchmark_csv("benchmark_results.csv");
    printf("+=========================================================================================+\n\n");
}

void run_stress_test_benchmark(int total_size, int num_processes) {
    printf("\n\033[1;37;45m ========================================================================================= \033[0m\n");
    printf("\033[1;37;45m  STRESS WORKLOAD MONTE-CARLO SIMULATION (%d RANDOM PROCESSES, %dB RAM)                  \033[0m\n",
           num_processes, total_size);
    printf("\033[1;37;45m ========================================================================================= \033[0m\n");

    const char* names[] = {"First Fit", "Next Fit", "Best Fit", "Worst Fit"};
    AlgorithmType algos[] = {ALGO_FIRST_FIT, ALGO_NEXT_FIT, ALGO_BEST_FIT, ALGO_WORST_FIT};

    for (int a = 0; a < 4; a++) {
        init_memory(total_size);

        unsigned int seed = 42;
        int alloc_count = 0;

        for (int i = 1; i <= num_processes; i++) {
            seed = seed * 1103515245 + 12345;
            int size = 40 + (seed % 160);

            Process p = {.process_id = i, .size_needed = size};
            if (allocate_memory(p, algos[a])) {
                alloc_count++;
            }

            if (i % 3 == 0) {
                int victim_pid = (i / 3);
                free_memory(victim_pid);
            }
        }

        double success_pct = (num_processes > 0) ? (alloc_count * 100.0 / num_processes) : 0.0;
        double avg_steps = (alloc_count > 0) ? ((double)g_total_search_steps / alloc_count) : 0.0;

        int total_allocated = 0;
        int total_free = 0;
        int max_hole = 0;
        MemoryBlock* curr = memory_head;
        while (curr != NULL) {
            if (curr->is_allocated) total_allocated += curr->size;
            else {
                total_free += curr->size;
                if (curr->size > max_hole) max_hole = curr->size;
            }
            curr = curr->next;
        }

        double ext_frag = (total_free > 0) ? (1.0 - (double)max_hole / total_free) * 100.0 : 0.0;

        printf(" \033[1;33m[%s]\033[0m Success: \033[1;32m%d/%d (%.1f%%)\033[0m | Avg Search: \033[1;36m%.2f steps\033[0m | EFR: \033[1;35m%.1f%%\033[0m | Active RAM: %dB/%dB\n",
               names[a], alloc_count, num_processes, success_pct, avg_steps, ext_frag, total_allocated, total_size);

        cleanup_memory();
    }

    printf("+=========================================================================================+\n\n");
}
