#include <stdio.h>
#include <stdlib.h>
#include "memory_manager.h"

static void split_block(MemoryBlock* target, Process proc) {
    if (target == NULL || target->is_allocated || target->size < proc.size_needed) {
        printf("[LỖI] Khối nhớ không hợp lệ hoặc không đủ dung lượng để tách!\n");
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

int main(void) {
    printf("=====================================================================\n");
    printf("  DEMO BASE ARCHITECTURE: MÔ PHỎNG QUẢN LÝ BỘ NHỚ HỆ THỐNG TRONG C  \n");
    printf("=====================================================================\n\n");

    printf(">>> BƯỚC 1: Khởi tạo 1000 Bytes RAM ban đầu...\n");
    init_memory(1000);
    print_memory_state();

    printf(">>> BƯỚC 2: Mô phỏng cấp phát cho 3 tiến trình (P1: 200B, P2: 300B, P3: 250B)...\n");
    Process p1 = {.process_id = 1, .size_needed = 200};
    Process p2 = {.process_id = 2, .size_needed = 300};
    Process p3 = {.process_id = 3, .size_needed = 250};

    split_block(memory_head, p1);
    split_block(memory_head->next, p2);
    split_block(memory_head->next->next, p3);
    print_memory_state();

    printf(">>> BƯỚC 3: Giải phóng tiến trình P2 (PID = 2)...\n");
    free_memory(2);
    print_memory_state();

    printf(">>> BƯỚC 4: Giải phóng tiến trình P3 (PID = 3) để kích hoạt gộp nhiều khối liền kề...\n");
    free_memory(3);
    print_memory_state();

    printf(">>> BƯỚC 5: Giải phóng nốt tiến trình P1 (PID = 1)...\n");
    free_memory(1);
    print_memory_state();

    printf(">>> BƯỚC 6: Dọn dẹp tài nguyên hệ thống...\n");
    cleanup_memory();

    printf("\n[HOÀN TẤT] Chương trình mô phỏng chạy thành công!\n");
    return 0;
}
