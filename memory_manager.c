#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "memory_manager.h"

MemoryBlock* memory_head = NULL;
MemoryBlock* last_allocated_block = NULL;

static int g_total_memory_size = 0;

void init_memory(int total_size) {
    if (total_size <= 0) {
        printf("\033[1;31m[LỖI] Kích thước bộ nhớ khởi tạo phải lớn hơn 0!\033[0m\n");
        return;
    }

    if (memory_head != NULL) {
        cleanup_memory();
    }

    MemoryBlock* initial_block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    if (initial_block == NULL) {
        printf("\033[1;31m[LỖI NGHIÊM TRỌNG] Không thể cấp phát bộ nhớ cho MemoryBlock!\033[0m\n");
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
}

void render_htop_view(const char* current_event) {
    if (memory_head == NULL) {
        printf("[CẢNH BÁO] Bộ nhớ chưa được khởi tạo!\n");
        return;
    }

    int total_allocated = 0;
    int total_free = 0;
    int total_blocks = 0;
    int active_processes = 0;

    MemoryBlock* curr = memory_head;
    while (curr != NULL) {
        total_blocks++;
        if (curr->is_allocated) {
            total_allocated += curr->size;
            active_processes++;
        } else {
            total_free += curr->size;
        }
        curr = curr->next;
    }

    printf("\033[H\033[J");

    printf("\033[1;37;44m  HTOP - SIMULATION MEMORY MANAGER [ALGO: NEXT FIT]                       \033[0m\n");

    int gauge_width = 40;
    int filled_gauge = 0;
    if (g_total_memory_size > 0) {
        filled_gauge = (total_allocated * gauge_width) / g_total_memory_size;
    }

    printf("\033[1;36mMem\033[0m  [");
    for (int i = 0; i < gauge_width; i++) {
        if (i < filled_gauge) {
            if (i < gauge_width * 0.6) {
                printf("\033[1;32m|\033[0m");
            } else if (i < gauge_width * 0.85) {
                printf("\033[1;33m|\033[0m");
            } else {
                printf("\033[1;31m|\033[0m");
            }
        } else {
            printf("\033[1;30m.\033[0m");
        }
    }
    double percent = g_total_memory_size > 0 ? (total_allocated * 100.0 / g_total_memory_size) : 0.0;
    printf("] \033[1;37m%d\033[0m/\033[1;32m%dB\033[0m (\033[1;33m%.1f%%\033[0m)\n",
           total_allocated, g_total_memory_size, percent);

    int next_fit_addr = last_allocated_block ? last_allocated_block->start_address : 0;
    int next_fit_pid = last_allocated_block ? last_allocated_block->process_id : -1;
    printf("\033[1;36mStats\033[0m Blocks: \033[1;33m%d\033[0m | Procs: \033[1;32m%d\033[0m | Free: \033[1;36m%dB\033[0m | NextFit Pointer: \033[1;35m@Address %d (PID:%d)\033[0m\n",
           total_blocks, active_processes, total_free, next_fit_addr, next_fit_pid);

    printf("\n\033[1;37m[ RAM PHYSICAL LAYOUT MAP ]\033[0m\n");
    printf("+");
    for (int i = 0; i < 68; i++) printf("-");
    printf("+\n|");

    int bar_width = 68;
    curr = memory_head;
    int chars_printed = 0;
    while (curr != NULL) {
        int w = (curr->size * bar_width) / g_total_memory_size;
        if (w < 1) w = 1;
        if (chars_printed + w > bar_width || curr->next == NULL) {
            w = bar_width - chars_printed;
        }

        const char* color = "\033[47;30m";
        if (curr == last_allocated_block) {
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
            color = "\033[100;37m";
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
    for (int i = 0; i < 68; i++) printf("-");
    printf("+\n");

    printf("\033[1;30m0B");
    int quarter = g_total_memory_size / 4;
    printf("%*dB%*dB%*dB%*dB\033[0m\n\n", 
           15, quarter, 17, quarter * 2, 17, quarter * 3, 15, g_total_memory_size);

    printf("\033[46;30m  PID   START ADDR   END ADDR     SIZE     STATUS        USAGE%%    POINTER      \033[0m\n");

    curr = memory_head;
    while (curr != NULL) {
        int end_addr = curr->start_address + curr->size - 1;
        double block_pct = (curr->size * 100.0) / g_total_memory_size;
        bool is_pointer = (curr == last_allocated_block);

        if (is_pointer) {
            printf("\033[1;33m");
        }

        if (curr->is_allocated) {
            printf("  %-5d %-12d %-12d %-8dB \033[1;32m%-12s\033[0m%s %5.1f%%    %s\n",
                   curr->process_id,
                   curr->start_address,
                   end_addr,
                   curr->size,
                   "ALLOCATED",
                   is_pointer ? "\033[1;33m" : "",
                   block_pct,
                   is_pointer ? "<-- [LAST ALLOC]" : "");
        } else {
            printf("  %-5s %-12d %-12d %-8dB \033[1;34m%-12s\033[0m%s %5.1f%%    %s\n",
                   "FREE",
                   curr->start_address,
                   end_addr,
                   curr->size,
                   "IDLE/FREE",
                   is_pointer ? "\033[1;33m" : "",
                   block_pct,
                   is_pointer ? "<-- [LAST ALLOC]" : "");
        }

        if (is_pointer) {
            printf("\033[0m");
        }

        curr = curr->next;
    }

    printf("\n\033[1;37;40m>>> STATUS LOG: %s\033[0m\n", current_event ? current_event : "Ready");
    fflush(stdout);
}

void print_memory_state(void) {
    render_htop_view("Snapshot updated");
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

bool allocate_next_fit(Process proc) {
    if (memory_head == NULL || proc.process_id <= 0 || proc.size_needed <= 0) {
        return false;
    }

    if (last_allocated_block == NULL) {
        last_allocated_block = memory_head;
    }

    MemoryBlock* start = last_allocated_block;
    MemoryBlock* current = start;
    bool wrapped_around = false;

    while (current != NULL) {
        if (!current->is_allocated && current->size >= proc.size_needed) {
            split_block(current, proc);
            last_allocated_block = current;
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

    return false;
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
}
