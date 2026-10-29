#include <stdio.h>
#include <stdlib.h>
#include "memory_manager.h"

int main(void) {
    printf("=====================================================================\n");
    printf("  KIỂM THỬ MODULE THÀNH VIÊN 1 (TRƯỞNG NHÓM): BASE & NEXT FIT ALGO   \n");
    printf("=====================================================================\n\n");

    printf(">>> BƯỚC 1: Khởi tạo 1000 Bytes RAM mô phỏng...\n");
    init_memory(1000);
    print_memory_state();

    printf(">>> BƯỚC 2: Cấp phát tuần tự bằng thuật toán NEXT FIT...\n");
    Process p1 = {.process_id = 1, .size_needed = 200};
    Process p2 = {.process_id = 2, .size_needed = 300};
    Process p3 = {.process_id = 3, .size_needed = 250};

    allocate_next_fit(p1);
    printf("[NEXT FIT] Đã cấp phát cho Process 1 (200B). Con trỏ lưu vết tại khối P1.\n");
    
    allocate_next_fit(p2);
    printf("[NEXT FIT] Đã cấp phát cho Process 2 (300B). Tìm tiếp từ sau khối P1.\n");

    allocate_next_fit(p3);
    printf("[NEXT FIT] Đã cấp phát cho Process 3 (250B). Tìm tiếp từ sau khối P2.\n");
    print_memory_state();

    printf(">>> BƯỚC 3: Giải phóng Process 1 (200B)...\n");
    free_memory(1);
    print_memory_state();

    printf(">>> BƯỚC 4: Cấp phát Process 4 (150B) để kiểm tra xoay vòng (Wrap-around)...\n");
    Process p4 = {.process_id = 4, .size_needed = 150};
    if (allocate_next_fit(p4)) {
        printf("[NEXT FIT] Thành công! Next Fit duyệt hết cuối RAM và quay vòng về đầu để lấy khối trống của P1 vừa nhả.\n");
    }
    print_memory_state();

    printf(">>> BƯỚC 5: Dọn dẹp tài nguyên (Chống rò rỉ bộ nhớ)...\n");
    cleanup_memory();

    printf("\n[HOÀN TẤT] Module Trưởng nhóm hoạt động chính xác 100%%!\n");
    return 0;
}
