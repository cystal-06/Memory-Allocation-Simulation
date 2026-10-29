#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "memory_manager.h"

static void run_realtime_demo(void) {
    init_memory(1000);
    render_htop_view("[KERNEL_BOOT] Initialized 1000B contiguous physical RAM pool. Boundary: [0x0000 - 0x03E7].");
    usleep(1100000);

    Process p1 = {.process_id = 101, .size_needed = 200};
    allocate_next_fit(p1);
    render_htop_view("[SYS_ALLOC] Next-Fit dispatched PID 101 (200B) at 0x0000. Partition split: [Alloc 200B | Remainder 800B].");
    usleep(1100000);

    Process p2 = {.process_id = 102, .size_needed = 300};
    allocate_next_fit(p2);
    render_htop_view("[SYS_ALLOC] Next-Fit dispatched PID 102 (300B) starting from PTR_LAST (0x00C8). Partition split: [Alloc 300B | Remainder 500B].");
    usleep(1100000);

    Process p3 = {.process_id = 103, .size_needed = 250};
    allocate_next_fit(p3);
    render_htop_view("[SYS_ALLOC] Next-Fit dispatched PID 103 (250B) at 0x01F4. High memory pressure: 750B / 1000B allocated.");
    usleep(1100000);

    free_memory(101);
    render_htop_view("[SYS_FREE] Interrupted PID 101. Released 200B at 0x0000 -> Created Hole #1 (External Fragmentation induced).");
    usleep(1100000);

    Process p4 = {.process_id = 104, .size_needed = 150};
    allocate_next_fit(p4);
    render_htop_view("[SYS_ALLOC] Next-Fit dispatched PID 104 (150B) into tail segment [0x02EE - 0x0383]. Sliced tail hole to 100B.");
    usleep(1100000);

    Process p5 = {.process_id = 105, .size_needed = 180};
    allocate_next_fit(p5);
    render_htop_view("[CIRCULAR_WRAP_AROUND] Scanned to EOF -> Buffer end reached -> WRAP_AROUND triggered to 0x0000! Allocated PID 105 (180B).");
    usleep(1100000);

    free_memory(102);
    render_htop_view("[SYS_FREE] Interrupted PID 102. Released 300B at 0x00C8. Completed real-time workload simulation benchmark.");

    printf("\n\033[1;32m[SYSTEM READY] Demo simulation completed. Press Enter to continue...\033[0m\n");
    getchar();
    cleanup_memory();
}

static void run_interactive_shell(void) {
    int ram_size = 1000;
    init_memory(ram_size);

    int choice = -1;
    while (choice != 0) {
        printf("\n\033[1;36m=== MEMORY CONTROLLER INTERACTIVE SHELL ===\033[0m\n");
        printf("1. Allocate (First Fit)\n");
        printf("2. Allocate (Next Fit)\n");
        printf("3. Allocate (Best Fit)\n");
        printf("4. Allocate (Worst Fit)\n");
        printf("5. Free Process by PID\n");
        printf("6. Explicit Coalesce Free Memory Blocks\n");
        printf("7. Render HTOP Visual Dashboard\n");
        printf("8. View Evaluation & Statistics Report \n");
        printf("0. Return to Main Menu\n");
        printf("Enter selection: ");
        if (scanf("%d", &choice) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

        if (choice >= 1 && choice <= 4) {
            Process p;
            printf("Enter Process ID (> 0): ");
            scanf("%d", &p.process_id);
            printf("Enter Memory Needed (Bytes): ");
            scanf("%d", &p.size_needed);

            AlgorithmType algo = (AlgorithmType)(choice - 1);
            bool ok = allocate_memory(p, algo);
            if (ok) {
                render_htop_view("[INTERACTIVE] Process allocated successfully.");
            } else {
                printf("\033[1;31m[ERROR] Allocation rejected. PID duplicate or out of suitable memory blocks.\033[0m\n");
            }
        } else if (choice == 5) {
            int pid;
            printf("Enter Process ID to Free: ");
            scanf("%d", &pid);
            free_memory(pid);
            render_htop_view("[INTERACTIVE] Process released and coalesced.");
        } else if (choice == 6) {
            coalesce_memory();
            render_htop_view("[INTERACTIVE] Adjacent free blocks coalesced.");
        } else if (choice == 7) {
            render_htop_view("[INTERACTIVE] Current state inspected.");
        } else if (choice == 8) {
            print_evaluation_report();
        }
    }
    cleanup_memory();
}

int main(void) {
    int opt = -1;
    while (opt != 0) {
        printf("\n\033[1;37;44m ==================================================================== \033[0m\n");
        printf("\033[1;37;44m   MEMORY ALLOCATION SIMULATION & EVALUATION SYSTEM IN C              \033[0m\n");
        printf("\033[1;37;44m ==================================================================== \033[0m\n");
        printf(" \033[1;33m1.\033[0m Run Live HTOP Visual Demo (Real-time Next-Fit Animation)\n");
        printf(" \033[1;33m2.\033[0m Run 4-Algorithm Comparative Benchmark & Export CSV\n");
        printf(" \033[1;33m3.\033[0m Run Monte-Carlo Stress Test (50 Randomized Processes)\n");
        printf(" \033[1;33m4.\033[0m Launch Interactive CLI Memory Controller Shell\n");
        printf(" \033[1;31m0.\033[0m Exit Program\n");
        printf("--------------------------------------------------------------------\n");
        printf("Select mode [0-4]: ");

        if (scanf("%d", &opt) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

        getchar();

        switch (opt) {
            case 1:
                run_realtime_demo();
                break;
            case 2:
                run_comparison_benchmark(1000);
                printf("Press Enter to return to menu...");
                getchar();
                break;
            case 3:
                run_stress_test_benchmark(2000, 50);
                printf("Press Enter to return to menu...");
                getchar();
                break;
            case 4:
                run_interactive_shell();
                break;
            case 0:
                printf("\033[1;32mExiting memory simulator. Goodbye!\033[0m\n");
                break;
            default:
                printf("\033[1;31mInvalid option. Please choose [0-4].\033[0m\n");
                break;
        }
    }

    return 0;
}
