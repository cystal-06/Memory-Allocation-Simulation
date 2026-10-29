# Simulation and Evaluation of Memory Allocation Algorithms in C

Mô phỏng và Đánh giá các Thuật toán Cấp phát Bộ nhớ (First Fit, Next Fit, Best Fit, Worst Fit) bằng ngôn ngữ C.

## 1. Giới thiệu Đề tài
Dự án mô phỏng cơ chế quản lý bộ nhớ theo kỹ thuật **Phân vùng động liên tục (Contiguous Dynamic Memory Partitioning)** trong Hệ điều hành:
- Giả lập thanh RAM logic mà không gọi các System Call của hệ điều hành thực tế (`mmap`, `brk`).
- Quản lý các phân vùng bộ nhớ bằng cấu trúc **Danh sách liên kết đơn (Singly Linked List)** có thứ tự tăng dần theo địa chỉ (`start_address`).
- Thực hiện kỹ thuật **Tách khối (Block Splitting)** khi cấp phát và **Gom cụm (Coalescing)** khi giải phóng để chống hiện tượng phân mảnh ngoại vi (External Fragmentation).

---

## 2. Kiến trúc Nền tảng (Base Architecture)
Hệ thống sử dụng các cấu trúc dữ liệu cốt lõi trong `memory_manager.h`:
- `Process`: Đại diện cho tiến trình yêu cầu bộ nhớ (`process_id`, `size_needed`).
- `MemoryBlock`: Node của danh sách liên kết đại diện cho từng phân vùng nhớ (`start_address`, `size`, `is_allocated`, `process_id`, `next`).
- `memory_head`: Con trỏ đầu danh sách quản lý toàn bộ không gian địa chỉ.
- `last_allocated_block`: Con trỏ lưu vết vị trí cấp phát gần nhất (phục vụ thuật toán Next Fit).

---

## 3. Phân công Nhiệm vụ Nhóm

| Thành viên | Thuật toán đảm nhiệm | Module chức năng |
| :--- | :--- | :--- |
| **Thành viên 1 (Trưởng nhóm)** | **Next Fit** (Lưu vết con trỏ & Xoay vòng) | **Base Architecture** (`init`, `print`, `free`, `cleanup`) & Quản trị Git |
| **Thành viên 2** | **Best Fit** (Tìm khối nhỏ nhất vừa đủ) | **Gom cụm bộ nhớ (Coalescing)** chống phân mảnh |
| **Thành viên 3** | **Worst Fit** (Tìm khối lớn nhất) | **Thống kê & Đánh giá (Evaluation)** hiệu năng |
| **Thành viên 4** | **First Fit** (Tìm khối đầu tiên vừa đủ) | **Giao diện Console (CLI Menu)** & Sinh dữ liệu tự động |

---

## 4. Hướng dẫn Biên dịch và Chạy thử

### Yêu cầu hệ thống:
- Trình biên dịch C: `gcc` hỗ trợ chuẩn C99 trở lên.
- Công cụ build: `make`.

### Lệnh thực thi:
```bash
# Biên dịch và chạy chương trình thử nghiệm:
make run

# Dọn dẹp các file nhị phân tạm (.o, memory_sim):
make clean
```

Hoặc biên dịch thủ công bằng `gcc`:
```bash
gcc -Wall -Wextra -std=c99 main.c memory_manager.c -o memory_sim
./memory_sim
```

---

## 5. Quy chuẩn Phân nhánh Git (Git Workflow cho Nhóm)
1. Nhánh `main`: Nhánh ổn định chính thức, do Trưởng nhóm quản lý và kiểm duyệt.
2. Các thành viên clone từ nhánh `main` và tạo nhánh riêng để làm việc:
   - Thành viên 2: `git checkout -b feature/bestfit-coalesce`
   - Thành viên 3: `git checkout -b feature/worstfit-evaluation`
   - Thành viên 4: `git checkout -b feature/firstfit-cli`
3. Khi hoàn thành module, các thành viên đẩy lên nhánh của mình và tạo Pull Request (PR) để Trưởng nhóm review và merge vào `main`.
