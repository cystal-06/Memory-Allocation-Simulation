#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "memory_manager.h"

static void delay_animation(void) {
    usleep(1300000);
}

int main(void) {
    init_memory(1000);
    render_htop_view("[KERNEL_BOOT] Initialized 1000B contiguous physical RAM pool. Boundary: [0x0000 - 0x03E7].");
    delay_animation();

    Process p1 = {.process_id = 101, .size_needed = 200};
    allocate_next_fit(p1);
    render_htop_view("[SYS_ALLOC] Next-Fit dispatched PID 101 (200B) at 0x0000. Partition split: [Alloc 200B | Remainder 800B].");
    delay_animation();

    Process p2 = {.process_id = 102, .size_needed = 300};
    allocate_next_fit(p2);
    render_htop_view("[SYS_ALLOC] Next-Fit dispatched PID 102 (300B) starting from PTR_LAST (0x00C8). Partition split: [Alloc 300B | Remainder 500B].");
    delay_animation();

    Process p3 = {.process_id = 103, .size_needed = 250};
    allocate_next_fit(p3);
    render_htop_view("[SYS_ALLOC] Next-Fit dispatched PID 103 (250B) at 0x01F4. High memory pressure: 750B / 1000B allocated.");
    delay_animation();

    free_memory(101);
    render_htop_view("[SYS_FREE] Interrupted PID 101. Released 200B at 0x0000 -> Created Hole #1 (External Fragmentation induced).");
    delay_animation();

    Process p4 = {.process_id = 104, .size_needed = 150};
    allocate_next_fit(p4);
    render_htop_view("[SYS_ALLOC] Next-Fit dispatched PID 104 (150B) into tail segment [0x02EE - 0x0383]. Sliced tail hole to 100B.");
    delay_animation();

    Process p5 = {.process_id = 105, .size_needed = 180};
    allocate_next_fit(p5);
    render_htop_view("[CIRCULAR_WRAP_AROUND] Scanned to EOF -> Buffer end reached -> WRAP_AROUND triggered to 0x0000! Allocated PID 105 (180B).");
    delay_animation();

    free_memory(102);
    render_htop_view("[SYS_FREE] Interrupted PID 102. Released 300B at 0x00C8. Completed real-time workload simulation benchmark.");

    printf("\n\033[1;32m[SYSTEM READY] Simulation cycle completed. Press Enter to exit...\033[0m\n");
    getchar();

    cleanup_memory();
    return 0;
}
