#include <stdio.h>
#include <stdlib.h>
#include "memory_manager.h"

MemoryBlock* memory_head = NULL;

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

void coalesce_memory(void) {
    if (memory_head == NULL) {
        return;
    }

    MemoryBlock* current = memory_head;
    int merge_count = 0;

    while (current != NULL && current->next != NULL) {
        if (!current->is_allocated && !current->next->is_allocated) {
            MemoryBlock* duplicate_node = current->next;

            current->size += duplicate_node->size;
            current->next = duplicate_node->next;
            free(duplicate_node);

            merge_count++;
        } else {
            current = current->next;
        }
    }

    if (merge_count > 0) {
        printf("[GOM CỤM] Đã thực hiện %d lần gộp các khối nhớ trống liền kề.\n", merge_count);
    }
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
        }
        current = current->next;
    }

    if (!found) {
        printf("[CẢNH BÁO] Không tìm thấy khối nhớ nào thuộc về Process ID: %d!\n", process_id);
        return;
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
    printf("[HỆ THỐNG] Đã giải phóng toàn bộ tài nguyên cấu trúc bộ nhớ.\n");
}
