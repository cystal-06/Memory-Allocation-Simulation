#include <stdio.h>
#include "memory_manager.h"

int main(void) {
    printf("=====================================================================\n");
    printf("  MÔ PHỎNG BEST FIT VÀ GOM CỤM BỘ NHỚ TRONG C                 \n");
    printf("=====================================================================\n\n");

    printf(">>> BƯỚC 1: Khởi tạo 1000 Bytes RAM ban đầu...\n");
    init_memory(1000);
    print_memory_state();

    printf(">>> BƯỚC 2: Cấp phát bằng Best Fit (P1: 200B, P2: 300B, P3: 250B)...\n");
    Process p1 = {.process_id = 1, .size_needed = 200};
    Process p2 = {.process_id = 2, .size_needed = 300};
    Process p3 = {.process_id = 3, .size_needed = 250};

    allocate_best_fit(p1);
    allocate_best_fit(p2);
    allocate_best_fit(p3);
    print_memory_state();

    printf(">>> BƯỚC 3: Giải phóng tiến trình P2 (PID = 2)...\n");
    free_memory(2);
    print_memory_state();

    printf(">>> BƯỚC 4: Cấp phát P4 (200B). Best Fit phải chọn khối trống 250B thay vì 300B...\n");
    Process p4 = {.process_id = 4, .size_needed = 200};
    allocate_best_fit(p4);
    print_memory_state();

    printf(">>> BƯỚC 5: Giải phóng P3 và P4 để kích hoạt gộp nhiều khối liền kề...\n");
    free_memory(3);
    free_memory(4);
    print_memory_state();

    printf(">>> BƯỚC 6: Giải phóng nốt tiến trình P1 (PID = 1)...\n");
    free_memory(1);
    print_memory_state();

    printf(">>> BƯỚC 7: Dọn dẹp tài nguyên hệ thống...\n");
    cleanup_memory();

    printf("\n[HOÀN TẤT] Chương trình mô phỏng chạy thành công!\n");
    return 0;
}
