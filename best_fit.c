#include <stdio.h>
#include <stdlib.h>
#include "memory_manager.h"

static MemoryBlock* find_best_fit_block(int size_needed) {
    MemoryBlock* current = memory_head;
    MemoryBlock* selected = NULL;

    while (current != NULL) {
        if (!current->is_allocated && current->size >= size_needed
            && (selected == NULL || current->size < selected->size)) {
            selected = current;
        }
        current = current->next;
    }
    return selected;
}

static bool split_block(MemoryBlock* target, Process process) {
    if (target == NULL || target->is_allocated || target->size < process.size_needed) {
        return false;
    }

    if (target->size != process.size_needed) {
        MemoryBlock* remainder = (MemoryBlock*)malloc(sizeof(MemoryBlock));
        if (remainder == NULL) {
            printf("[LỖI NGHIÊM TRỌNG] Không đủ bộ nhớ Heap để tạo node phần dư!\n");
            return false;
        }
        remainder->start_address = target->start_address + process.size_needed;
        remainder->size = target->size - process.size_needed;
        remainder->is_allocated = false;
        remainder->process_id = -1;
        remainder->next = target->next;
        target->next = remainder;
    }

    target->size = process.size_needed;
    target->is_allocated = true;
    target->process_id = process.process_id;
    return true;
}

bool allocate_best_fit(Process process) {
    MemoryBlock* selected;
    MemoryBlock* current;

    if (memory_head == NULL) {
        printf("[CẢNH BÁO] Bộ nhớ chưa được khởi tạo!\n");
        return false;
    }
    if (process.process_id < 0 || process.size_needed <= 0) {
        printf("[LỖI] PID phải không âm và kích thước tiến trình phải lớn hơn 0!\n");
        return false;
    }

    current = memory_head;
    while (current != NULL) {
        if (current->is_allocated && current->process_id == process.process_id) {
            printf("[CẢNH BÁO] PID %d đã tồn tại trong bộ nhớ!\n", process.process_id);
            return false;
        }
        current = current->next;
    }

    selected = find_best_fit_block(process.size_needed);
    if (selected == NULL) {
        printf("[CẤP PHÁT THẤT BẠI] Không có khối trống phù hợp cho PID %d (%d Bytes) bằng Best Fit.\n",
               process.process_id, process.size_needed);
        return false;
    }
    if (!split_block(selected, process)) {
        printf("[CẤP PHÁT THẤT BẠI] Không thể tách khối cho PID %d.\n", process.process_id);
        return false;
    }

    printf("[CẤP PHÁT] PID %d: %d Bytes bằng Best Fit tại địa chỉ %d.\n",
           process.process_id, process.size_needed, selected->start_address);
    return true;
}

void coalesce_memory(void) {
    MemoryBlock* current = memory_head;
    int merge_count = 0;

    while (current != NULL && current->next != NULL) {
        bool blocks_are_adjacent =
            current->start_address + current->size == current->next->start_address;
        if (!current->is_allocated && !current->next->is_allocated && blocks_are_adjacent) {
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
