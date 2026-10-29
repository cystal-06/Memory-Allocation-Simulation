#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "memory_manager.h"

static void delay_animation(void) {
    usleep(1400000);
}

int main(void) {
    init_memory(1000);
    render_htop_view("Khoi tao RAM 1000 Bytes ban dau thanh cong.");
    delay_animation();

    Process p1 = {.process_id = 1, .size_needed = 200};
    allocate_next_fit(p1);
    render_htop_view("[EVENT] Next Fit: Cap phat Process 1 (200B) tai dia chi 0.");
    delay_animation();

    Process p2 = {.process_id = 2, .size_needed = 300};
    allocate_next_fit(p2);
    render_htop_view("[EVENT] Next Fit: Cap phat Process 2 (300B) tiep tuc sau P1 (tai dia chi 200).");
    delay_animation();

    Process p3 = {.process_id = 3, .size_needed = 250};
    allocate_next_fit(p3);
    render_htop_view("[EVENT] Next Fit: Cap phat Process 3 (250B) tiep tuc sau P2 (tai dia chi 500).");
    delay_animation();

    free_memory(1);
    render_htop_view("[EVENT] Giai phong Process 1 (200B). Dia chi 0-199 tro thanh FREE!");
    delay_animation();

    Process p4 = {.process_id = 4, .size_needed = 150};
    allocate_next_fit(p4);
    render_htop_view("[EVENT] Next Fit: Cap phat Process 4 (150B) vao khoi cuoi cung (750-899).");
    delay_animation();

    Process p5 = {.process_id = 5, .size_needed = 180};
    allocate_next_fit(p5);
    render_htop_view("[EVENT] NEXT FIT WRAP-AROUND! Quet toi cuoi RAM khong du -> Xoay vong ve dau RAM cap phat P5 (180B) vao vung P1 cu!");
    delay_animation();

    free_memory(2);
    render_htop_view("[EVENT] Giai phong Process 2 (300B) tai dia chi 200. Hoan tat chu trinh mo phong.");

    printf("\n\033[1;32m[XONG] Mo phong thoi gian thuc hoan tat. Nhan Enter de ket thuc...\033[0m\n");
    getchar();

    cleanup_memory();
    return 0;
}
