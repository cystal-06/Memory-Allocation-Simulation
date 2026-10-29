#include <stdio.h>
#include <stdlib.h>
#include "memory_manager.h"

MemoryBlock* memory_head = NULL;
MemoryBlock* last_allocated_block = NULL;

void init_memory(int total_size) {
    if (total_size <= 0) {
        printf("[LỖI] Kích thước bộ nhớ khởi tạo phải lớn hơn 0!\n");
        return;
    }

    if (memory_head != NULL) {
        cleanup_memory();
    }

    MemoryBlock* initial_block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    if (initial_block == NULL) {
        printf("[LỖI NGHIÊM TRỌNG] Không thể cấp phát bộ nhớ cho MemoryBlock khởi tạo!\n");
        exit(EXIT_FAILURE);
    }

    initial_block->start_address = 0;
    initial_block->size = total_size;
    initial_block->is_allocated = false;
    initial_block->process_id = -1;
    initial_block->next = NULL;

    memory_head = initial_block;
    last_allocated_block = initial_block;

    printf("[HỆ THỐNG] Khởi tạo bộ nhớ mô phỏng thành công: Tổng dung lượng = %d Bytes.\n", total_size);
}

void print_memory_state(void) {
    if (memory_head == NULL) {
        printf("[CẢNH BÁO] Bộ nhớ chưa được khởi tạo!\n");
        return;
    }

    printf("\n+===========================================================================+\n");
    printf("|                          BẢN ĐỒ BỘ NHỚ HIỆN TẠI                           |\n");
    printf("+---------------+---------------+---------------+------------+--------------+\n");
    printf("| Địa chỉ đầu   | Địa chỉ cuối  | Kích thước    | Trạng thái | Process ID   |\n");
    printf("+---------------+---------------+---------------+------------+--------------+\n");

    MemoryBlock* current = memory_head;
    int total_allocated = 0;
    int total_free = 0;
    int block_count = 0;

    while (current != NULL) {
        block_count++;
        int end_address = current->start_address + current->size - 1;

        if (current->is_allocated) {
            printf("| %-13d | %-13d | %-13d | %-10s | PID: %-8d |\n",
                   current->start_address,
                   end_address,
                   current->size,
                   "ALLOCATED",
                   current->process_id);
            total_allocated += current->size;
        } else {
            printf("| %-13d | %-13d | %-13d | %-10s | %-12s |\n",
                   current->start_address,
                   end_address,
                   current->size,
                   "FREE",
                   "None (-1)");
            total_free += current->size;
        }

        current = current->next;
    }

    printf("+---------------+---------------+---------------+------------+--------------+\n");
    printf("| Tổng số block : %-5d                                                     |\n", block_count);
    printf("| Đã dùng       : %-5d Bytes | Còn trống: %-5d Bytes                         |\n", total_allocated, total_free);
    printf("+===========================================================================+\n\n");
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
        printf("[LỖI NGHIÊM TRỌNG] Không đủ bộ nhớ Heap để tạo node phần dư!\n");
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
        printf("[CẢNH BÁO] Bộ nhớ chưa được khởi tạo!\n");
        return;
    }

    MemoryBlock* current = memory_head;
    bool found = false;

    while (current != NULL) {
        if (current->is_allocated && current->process_id == process_id) {
            current->is_allocated = false;
            current->process_id = -1;
            found = true;
            printf("[GIẢI PHÓNG] Đã thu hồi khối nhớ tại địa chỉ %d (kích thước %d Bytes) của PID %d.\n",
                   current->start_address, current->size, process_id);
            if (last_allocated_block == current) {
                last_allocated_block = memory_head;
            }
        }
        current = current->next;
    }

    if (!found) {
        printf("[CẢNH BÁO] Không tìm thấy khối nhớ nào thuộc về Process ID: %d!\n", process_id);
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
    printf("[HỆ THỐNG] Đã giải phóng toàn bộ tài nguyên cấu trúc bộ nhớ.\n");
}
