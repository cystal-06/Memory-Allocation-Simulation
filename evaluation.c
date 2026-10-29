#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "memory_manager.h"
#include "evaluation.h"

/* ========================================================================= */
/* THÀNH VIÊN 3: THUẬT TOÁN WORST FIT & CORE MODULE THỐNG KÊ - ĐÁNH GIÁ       */
/* Tệp: evaluation.c                                                         */
/* ========================================================================= */

// Định nghĩa các biến viễn trắc toàn cục được chia sẻ trong hệ thống
int g_total_alloc_requests = 0;
int g_successful_allocs = 0;
int g_total_search_steps = 0;

static BenchmarkResult g_last_benchmark_results[4];
static bool g_has_benchmark_results = false;

/* ========================================================================= */
/* 1. THUẬT TOÁN WORST FIT (TÌM KHỐI NHỚ TRỐNG LỚN NHẤT)                     */
/* ========================================================================= */
/**
 * @brief Cài đặt thuật toán cấp phát bộ nhớ Worst Fit.
 *
 * Nguyên lý hoạt động:
 *  - Quét toàn bộ danh sách liên kết các phân vùng nhớ (MemoryBlock linked list) từ memory_head.
 *  - Tìm kiếm phân vùng tự do (is_allocated == false) có kích thước LỚN NHẤT (max_size)
 *    và thỏa mãn điều kiện đủ chứa tiến trình (curr->size >= proc.size_needed).
 *  - Mục tiêu: Cắt lấy phần bộ nhớ cần thiết từ lỗ trống lớn nhất để phần dư còn lại (remainder)
 *    vẫn duy trì kích thước tương đối lớn, đủ cho các tiến trình kế tiếp sử dụng.
 *  - Phân tích độ phức tạp:
 *      + Thời gian: O(N) do bắt buộc phải duyệt qua toàn bộ N node trong RAM để tìm cực đại toàn cục.
 *      + Không gian: O(1) chỉ sử dụng các con trỏ tạm trên stack.
 */
bool allocate_worst_fit(Process proc) {
    g_total_alloc_requests++;
    if (memory_head == NULL || proc.process_id <= 0 || proc.size_needed <= 0 || is_pid_active(proc.process_id)) {
        return false;
    }

    MemoryBlock* curr = memory_head;
    MemoryBlock* worst_block = NULL;
    int max_size = -1;
    int steps = 0;

    while (curr != NULL) {
        steps++;
        if (!curr->is_allocated && curr->size >= proc.size_needed) {
            if (curr->size > max_size) {
                max_size = curr->size;
                worst_block = curr;
            }
        }
        curr = curr->next;
    }

    g_total_search_steps += steps;
    if (worst_block != NULL) {
        split_block(worst_block, proc);
        g_successful_allocs++;
        return true;
    }

    return false;
}

/* ========================================================================= */
/* 2. CORE MODULE: THỐNG KÊ & ĐÁNH GIÁ (EVALUATION MODULE)                   */
/* ========================================================================= */

/**
 * @brief Lấy tổng số yêu cầu cấp phát bộ nhớ đã tiếp nhận.
 */
int get_total_alloc_requests(void) {
    return g_total_alloc_requests;
}

/**
 * @brief Lấy số lượng tiến trình cấp phát bộ nhớ thành công.
 */
int get_successful_allocs(void) {
    return g_successful_allocs;
}

/**
 * @brief Lấy số lượng tiến trình cấp phát bộ nhớ thất bại.
 */
int get_failed_allocs(void) {
    return (g_total_alloc_requests >= g_successful_allocs) ? (g_total_alloc_requests - g_successful_allocs) : 0;
}

/**
 * @brief Tính toán tỷ lệ phần trăm cấp phát thành công (Success Rate %).
 */
double get_allocation_success_rate(void) {
    if (g_total_alloc_requests <= 0) {
        return 0.0;
    }
    return ((double)g_successful_allocs * 100.0) / (double)g_total_alloc_requests;
}

/**
 * @brief Tính tổng dung lượng RAM đang được các tiến trình chiếm dụng.
 */
int get_total_allocated_memory(void) {
    int total_allocated = 0;
    MemoryBlock* curr = memory_head;
    while (curr != NULL) {
        if (curr->is_allocated) {
            total_allocated += curr->size;
        }
        curr = curr->next;
    }
    return total_allocated;
}

/**
 * @brief Tính tổng dung lượng bộ nhớ trống (Free Holes) hiện có trong RAM.
 */
int get_total_free_memory(void) {
    int total_free = 0;
    MemoryBlock* curr = memory_head;
    while (curr != NULL) {
        if (!curr->is_allocated) {
            total_free += curr->size;
        }
        curr = curr->next;
    }
    return total_free;
}

/**
 * @brief Tìm kích thước của khối nhớ trống lớn nhất hiện tại (Max Free Hole).
 */
int get_max_free_hole_size(void) {
    int max_hole = 0;
    MemoryBlock* curr = memory_head;
    while (curr != NULL) {
        if (!curr->is_allocated && curr->size > max_hole) {
            max_hole = curr->size;
        }
        curr = curr->next;
    }
    return max_hole;
}

/**
 * @brief Đếm số lượng khối nhớ trống (Holes) rời rạc trong RAM.
 */
int get_free_holes_count(void) {
    int count = 0;
    MemoryBlock* curr = memory_head;
    while (curr != NULL) {
        if (!curr->is_allocated) {
            count++;
        }
        curr = curr->next;
    }
    return count;
}

/**
 * @brief Tính toán tỷ lệ phần trăm phân mảnh ngoài theo chỉ số chuẩn EFR (External Fragmentation Ratio).
 * 
 * Công thức chuẩn trong Hệ điều hành:
 *   EFR (%) = (1.0 - (Max Free Hole / Total Free Memory)) * 100.0
 * 
 * Ý nghĩa:
 *  - EFR = 0%: Toàn bộ bộ nhớ trống là một khối liên tục duy nhất (hoàn toàn không phân mảnh).
 *  - EFR > 0%: Bộ nhớ trống bị chia cắt thành nhiều khối rời rạc. Càng gần 100%,
 *              các mẩu vụn càng phân tán, gây nguy cơ từ chối cấp phát dù tổng RAM trống lớn.
 */
double calculate_external_fragmentation(void) {
    int total_free = get_total_free_memory();
    int max_hole = get_max_free_hole_size();
    if (total_free <= 0) {
        return 0.0;
    }
    return (1.0 - (double)max_hole / (double)total_free) * 100.0;
}

/**
 * @brief Tính tỷ lệ phân mảnh ngoài so với TOÀN BỘ dung lượng RAM vật lý.
 * 
 * Công thức:
 *   ExtFrag_RAM (%) = ((Total Free Memory - Max Free Hole) / Total RAM Capacity) * 100.0
 */
double calculate_external_frag_total_ram(void) {
    int ram_size = get_total_memory_size();
    if (ram_size <= 0) {
        return 0.0;
    }
    int total_free = get_total_free_memory();
    int max_hole = get_max_free_hole_size();
    if (total_free <= max_hole) {
        return 0.0;
    }
    return ((double)(total_free - max_hole) * 100.0) / (double)ram_size;
}

/**
 * @brief Reset bộ đếm thống kê đánh giá hiệu năng.
 */
void reset_evaluation_stats(void) {
    g_total_alloc_requests = 0;
    g_successful_allocs = 0;
    g_total_search_steps = 0;
}

/**
 * @brief Xuất báo cáo thống kê và đánh giá hiệu năng hiện tại ra màn hình Terminal.
 */
void print_evaluation_report(void) {
    int ram_size = get_total_memory_size();
    int total_allocated = get_total_allocated_memory();
    int total_free = get_total_free_memory();
    int max_hole = get_max_free_hole_size();
    int holes_count = get_free_holes_count();
    int failed = get_failed_allocs();
    double success_rate = get_allocation_success_rate();
    double efr_pct = calculate_external_fragmentation();
    double efr_ram_pct = calculate_external_frag_total_ram();
    double mem_usage_pct = (ram_size > 0) ? ((double)total_allocated * 100.0 / ram_size) : 0.0;
    double avg_steps = (g_successful_allocs > 0) ? ((double)g_total_search_steps / g_successful_allocs) : 0.0;

    printf("\n\033[1;37;45m ========================================================================================= \033[0m\n");
    printf("\033[1;37;45m  CORE EVALUATION MODULE: REAL-TIME TELEMETRY & STATISTICAL REPORT (MEMBER 3)             \033[0m\n");
    printf("\033[1;37;45m ========================================================================================= \033[0m\n");
    printf(" \033[1;36m[PROCESS DISPATCH TELEMETRY]\033[0m\n");
    printf("  - Total Allocation Requests    : \033[1;37m%d\033[0m\n", g_total_alloc_requests);
    printf("  - Successful Allocations       : \033[1;32m%d\033[0m\n", g_successful_allocs);
    printf("  - Failed Allocations           : \033[1;31m%d\033[0m\n", failed);
    printf("  - Process Success Rate         : \033[1;33m%.2f%%\033[0m\n", success_rate);
    printf("  - Average Search Steps / Alloc : \033[1;36m%.2f steps\033[0m\n", avg_steps);
    printf("\n \033[1;36m[PHYSICAL RAM UTILIZATION & FRAGMENTATION]\033[0m\n");
    printf("  - Total RAM Capacity           : \033[1;37m%d Bytes\033[0m\n", ram_size);
    printf("  - In-Use Allocated Memory      : \033[1;32m%d Bytes\033[0m (%.1f%%)\n", total_allocated, mem_usage_pct);
    printf("  - Total Free Memory Available  : \033[1;34m%d Bytes\033[0m (%.1f%%)\n", total_free, 100.0 - mem_usage_pct);
    printf("  - Disjoint Free Holes Count    : \033[1;35m%d holes\033[0m\n", holes_count);
    printf("  - Largest Contiguous Free Hole : \033[1;33m%d Bytes\033[0m\n", max_hole);
    printf("  - External Fragmentation (EFR) : \033[1;31m%.2f%%\033[0m [Formula: 1 - (MaxHole / TotalFree)]\n", efr_pct);
    printf("  - Fragmented RAM Ratio / Total : \033[1;31m%.2f%%\033[0m [Formula: (TotalFree - MaxHole) / RAM]\n", efr_ram_pct);
    printf("-----------------------------------------------------------------------------------------\n");
    if (efr_pct > 40.0) {
        printf(" \033[1;33m[KERNEL DIAGNOSIS]\033[0m High external fragmentation detected! Recommend running \033[1;36mCoalesce (Option 6)\033[0m.\n");
    } else {
        printf(" \033[1;32m[KERNEL DIAGNOSIS]\033[0m Memory partitioning healthy. External fragmentation within acceptable bounds.\n");
    }
    printf("=========================================================================================\n\n");
}

/* ========================================================================= */
/* 3. BENCHMARK SO SÁNH HIỆU NĂNG 4 THUẬT TOÁN                              */
/* ========================================================================= */

BenchmarkResult evaluate_single_algo(AlgorithmType algo, const char* name, int ram_size) {
    init_memory(ram_size);

    Process test_suite[] = {
        {.process_id = 1, .size_needed = 120},
        {.process_id = 2, .size_needed = 180},
        {.process_id = 3, .size_needed = 90},
        {.process_id = 4, .size_needed = 220},
        {.process_id = 5, .size_needed = 150},
        {.process_id = 6, .size_needed = 80},
        {.process_id = 7, .size_needed = 110},
        {.process_id = 8, .size_needed = 70}
    };
    int total_procs = sizeof(test_suite) / sizeof(test_suite[0]);

    for (int i = 0; i < total_procs; i++) {
        allocate_memory(test_suite[i], algo);
    }

    free_memory(2);
    free_memory(4);
    free_memory(6);

    Process late_arrivals[] = {
        {.process_id = 9,  .size_needed = 100},
        {.process_id = 10, .size_needed = 130},
        {.process_id = 11, .size_needed = 160},
        {.process_id = 12, .size_needed = 75}
    };
    int late_count = sizeof(late_arrivals) / sizeof(late_arrivals[0]);

    for (int i = 0; i < late_count; i++) {
        allocate_memory(late_arrivals[i], algo);
    }

    int total_allocated = 0;
    int total_free = 0;
    int max_hole = 0;

    MemoryBlock* curr = memory_head;
    while (curr != NULL) {
        if (curr->is_allocated) {
            total_allocated += curr->size;
        } else {
            total_free += curr->size;
            if (curr->size > max_hole) {
                max_hole = curr->size;
            }
        }
        curr = curr->next;
    }

    BenchmarkResult res;
    res.algo = algo;
    res.algo_name = name;
    res.total_requests = g_total_alloc_requests;
    res.successful_allocs = g_successful_allocs;
    res.failed_allocs = g_total_alloc_requests - g_successful_allocs;
    res.total_search_steps = g_total_search_steps;
    res.avg_search_steps = (g_successful_allocs > 0) ? ((double)g_total_search_steps / g_successful_allocs) : 0.0;
    res.final_mem_usage_pct = (ram_size > 0) ? (total_allocated * 100.0 / ram_size) : 0.0;
    res.final_ext_frag_pct = (total_free > 0) ? (1.0 - (double)max_hole / total_free) * 100.0 : 0.0;
    res.max_hole_size = max_hole;

    cleanup_memory();
    return res;
}

void export_benchmark_csv(const char* filepath) {
    if (!g_has_benchmark_results) {
        return;
    }

    FILE* f = fopen(filepath, "w");
    if (f == NULL) {
        printf("\033[1;31m[ERROR] Unable to write benchmark CSV to %s\033[0m\n", filepath);
        return;
    }

    fprintf(f, "Algorithm,Total_Requests,Successful_Allocs,Failed_Allocs,Success_Rate_Pct,Avg_Search_Steps,Final_Mem_Usage_Pct,Final_Ext_Frag_Pct,Max_Hole_Bytes\n");
    for (int i = 0; i < 4; i++) {
        double s_rate = (g_last_benchmark_results[i].successful_allocs * 100.0) / g_last_benchmark_results[i].total_requests;
        fprintf(f, "%s,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%d\n",
                g_last_benchmark_results[i].algo_name,
                g_last_benchmark_results[i].total_requests,
                g_last_benchmark_results[i].successful_allocs,
                g_last_benchmark_results[i].failed_allocs,
                s_rate,
                g_last_benchmark_results[i].avg_search_steps,
                g_last_benchmark_results[i].final_mem_usage_pct,
                g_last_benchmark_results[i].final_ext_frag_pct,
                g_last_benchmark_results[i].max_hole_size);
    }
    fclose(f);
    printf("\033[1;32m[EXPORT SUCCESS] Benchmark telemetry saved to %s\033[0m\n", filepath);
}

void run_comparison_benchmark(int total_size) {
    g_last_benchmark_results[0] = evaluate_single_algo(ALGO_FIRST_FIT, "First Fit", total_size);
    g_last_benchmark_results[1] = evaluate_single_algo(ALGO_NEXT_FIT,  "Next Fit",  total_size);
    g_last_benchmark_results[2] = evaluate_single_algo(ALGO_BEST_FIT,  "Best Fit",  total_size);
    g_last_benchmark_results[3] = evaluate_single_algo(ALGO_WORST_FIT, "Worst Fit", total_size);
    g_has_benchmark_results = true;

    printf("\n\033[1;37;44m ========================================================================================= \033[0m\n");
    printf("\033[1;37;44m  SCIENTIFIC BENCHMARK REPORT: ALLOCATION STRATEGY PERFORMANCE EVALUATION                 \033[0m\n");
    printf("\033[1;37;44m ========================================================================================= \033[0m\n");
    printf(" Test Workload: 12 Mixed Processes (Alloc & Free holes) | Initial RAM: \033[1;32m%d Bytes\033[0m\n\n", total_size);

    printf("\033[46;30m ALGORITHM   TOTAL REQ   SUCCESS   FAILED   SUCCESS_RATE   AVG_STEPS   MEM_USAGE%%   EFR_FRAG%% \033[0m\n");

    for (int i = 0; i < 4; i++) {
        double s_rate = (g_last_benchmark_results[i].successful_allocs * 100.0) / g_last_benchmark_results[i].total_requests;
        printf(" \033[1;37m%-11s\033[0m %-11d \033[1;32m%-9d\033[0m \033[1;31m%-8d\033[0m \033[1;33m%11.1f%%\033[0m   \033[1;36m%9.2f\033[0m   \033[1;32m%9.1f%%\033[0m   \033[1;35m%8.1f%%\033[0m\n",
               g_last_benchmark_results[i].algo_name,
               g_last_benchmark_results[i].total_requests,
               g_last_benchmark_results[i].successful_allocs,
               g_last_benchmark_results[i].failed_allocs,
               s_rate,
               g_last_benchmark_results[i].avg_search_steps,
               g_last_benchmark_results[i].final_mem_usage_pct,
               g_last_benchmark_results[i].final_ext_frag_pct);
    }

    printf("+-----------------------------------------------------------------------------------------+\n");
    printf("\033[1;33m [ASCII VISUAL COMPARISON CHARTS]\033[0m\n\n");

    printf("  \033[1;36mSearch Overhead (Avg Steps - Lower is faster):\033[0m\n");
    for (int i = 0; i < 4; i++) {
        int bar_len = (int)(g_last_benchmark_results[i].avg_search_steps * 3.5);
        if (bar_len < 1) bar_len = 1;
        printf("  %-10s [", g_last_benchmark_results[i].algo_name);
        for (int k = 0; k < 30; k++) {
            if (k < bar_len) printf("\033[1;32m#\033[0m");
            else printf("\033[1;30m.\033[0m");
        }
        printf("] \033[1;37m%.2f steps\033[0m\n", g_last_benchmark_results[i].avg_search_steps);
    }

    printf("\n  \033[1;36mExternal Fragmentation Ratio (EFR - Lower is less fragmented):\033[0m\n");
    for (int i = 0; i < 4; i++) {
        int bar_len = (int)(g_last_benchmark_results[i].final_ext_frag_pct * 0.30);
        if (bar_len < 1) bar_len = 1;
        printf("  %-10s [", g_last_benchmark_results[i].algo_name);
        for (int k = 0; k < 30; k++) {
            if (k < bar_len) printf("\033[1;35m#\033[0m");
            else printf("\033[1;30m.\033[0m");
        }
        printf("] \033[1;37m%.1f%%\033[0m\n", g_last_benchmark_results[i].final_ext_frag_pct);
    }

    printf("+-----------------------------------------------------------------------------------------+\n");
    export_benchmark_csv("benchmark_results.csv");
    printf("+=========================================================================================+\n\n");
}

void run_stress_test_benchmark(int total_size, int num_processes) {
    printf("\n\033[1;37;45m ========================================================================================= \033[0m\n");
    printf("\033[1;37;45m  STRESS WORKLOAD MONTE-CARLO SIMULATION (%d RANDOM PROCESSES, %dB RAM)                  \033[0m\n",
           num_processes, total_size);
    printf("\033[1;37;45m ========================================================================================= \033[0m\n");

    const char* names[] = {"First Fit", "Next Fit", "Best Fit", "Worst Fit"};
    AlgorithmType algos[] = {ALGO_FIRST_FIT, ALGO_NEXT_FIT, ALGO_BEST_FIT, ALGO_WORST_FIT};

    for (int a = 0; a < 4; a++) {
        init_memory(total_size);

        unsigned int seed = 42;
        int alloc_count = 0;

        for (int i = 1; i <= num_processes; i++) {
            seed = seed * 1103515245 + 12345;
            int size = 40 + (seed % 160);

            Process p = {.process_id = i, .size_needed = size};
            if (allocate_memory(p, algos[a])) {
                alloc_count++;
            }

            if (i % 3 == 0) {
                int victim_pid = (i / 3);
                free_memory(victim_pid);
            }
        }

        double success_pct = (num_processes > 0) ? (alloc_count * 100.0 / num_processes) : 0.0;
        double avg_steps = (alloc_count > 0) ? ((double)g_total_search_steps / alloc_count) : 0.0;

        int total_allocated = 0;
        int total_free = 0;
        int max_hole = 0;
        MemoryBlock* curr = memory_head;
        while (curr != NULL) {
            if (curr->is_allocated) total_allocated += curr->size;
            else {
                total_free += curr->size;
                if (curr->size > max_hole) max_hole = curr->size;
            }
            curr = curr->next;
        }

        double ext_frag = (total_free > 0) ? (1.0 - (double)max_hole / total_free) * 100.0 : 0.0;

        printf(" \033[1;33m[%s]\033[0m Success: \033[1;32m%d/%d (%.1f%%)\033[0m | Avg Search: \033[1;36m%.2f steps\033[0m | EFR: \033[1;35m%.1f%%\033[0m | Active RAM: %dB/%dB\n",
               names[a], alloc_count, num_processes, success_pct, avg_steps, ext_frag, total_allocated, total_size);

        cleanup_memory();
    }

    printf("+=========================================================================================+\n\n");
}
