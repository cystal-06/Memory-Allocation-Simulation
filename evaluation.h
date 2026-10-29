#ifndef EVALUATION_H
#define EVALUATION_H

#include <stdbool.h>

/* ========================================================================= */
/* THÀNH VIÊN 3: THUẬT TOÁN WORST FIT & CORE MODULE THỐNG KÊ - ĐÁNH GIÁ       */
/* Tệp: evaluation.h                                                         */
/* ========================================================================= */

// Khai báo trước kiểu dữ liệu từ memory_manager.h để tránh phụ thuộc vòng
#include "memory_manager.h"

// Các biến viễn trắc toàn cục đo lường hiệu năng
extern int g_total_alloc_requests;
extern int g_successful_allocs;
extern int g_total_search_steps;

/* ------------------------------------------------------------------------- */
/* 1. THUẬT TOÁN WORST FIT (TÌM KHỐI NHỚ TRỐNG LỚN NHẤT)                     */
/* ------------------------------------------------------------------------- */
/**
 * @brief Cấp phát bộ nhớ cho tiến trình theo chiến lược Worst Fit.
 * @param proc Cấu trúc tiến trình chứa PID và dung lượng cần cấp phát.
 * @return true nếu tìm được khối trống lớn nhất và cấp phát thành công, false nếu thất bại.
 */
bool allocate_worst_fit(Process proc);

/* ------------------------------------------------------------------------- */
/* 2. CORE MODULE: THỐNG KÊ & ĐÁNH GIÁ (EVALUATION APIS)                    */
/* ------------------------------------------------------------------------- */
int get_total_alloc_requests(void);
int get_successful_allocs(void);
int get_failed_allocs(void);
double get_allocation_success_rate(void);

int get_total_allocated_memory(void);
int get_total_free_memory(void);
int get_max_free_hole_size(void);
int get_free_holes_count(void);

/**
 * @brief Tính tỷ lệ phần trăm phân mảnh ngoài theo chỉ số chuẩn EFR.
 * Công thức: EFR (%) = (1.0 - (Max Free Hole / Total Free Memory)) * 100.0
 */
double calculate_external_fragmentation(void);

/**
 * @brief Tính tỷ lệ phân mảnh ngoài trên toàn bộ dung lượng RAM vật lý.
 * Công thức: ExtFrag_RAM (%) = ((Total Free - Max Hole) / Total RAM) * 100.0
 */
double calculate_external_frag_total_ram(void);

void reset_evaluation_stats(void);
void print_evaluation_report(void);

/* ------------------------------------------------------------------------- */
/* 3. KHẢO SÁT SO SÁNH HIỆU NĂNG 4 THUẬT TOÁN & XUẤT CSV                     */
/* ------------------------------------------------------------------------- */
BenchmarkResult evaluate_single_algo(AlgorithmType algo, const char* name, int ram_size);
void run_comparison_benchmark(int total_size);
void run_stress_test_benchmark(int total_size, int num_processes);
void export_benchmark_csv(const char* filepath);

#endif // EVALUATION_H
