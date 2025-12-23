// ntfs.h — NTFS MFT 解析骨架（最小原型）
#pragma once

#include <cstdint>
#include <cstddef>

// 不透明句柄：表示一个打开的 NTFS/MFT 解析会话（实现细节隐藏）。
typedef struct ntfs_parser_s* ntfs_parser_t;

// 简化的 MFT 文件记录结构，用于迭代读取测试与原型功能。
typedef struct {
    uint64_t file_reference; // MFT 记录引用号
    uint64_t size;           // 文件大小（字节）
    uint64_t creation_time;  // UTC FILETIME 风格（原型）
    char file_name[256];     // 文件名（若可得）
} ntfs_file_record_t;

// 打开 MFT 解析会话
// 参数:
//  - image_path: 镜像文件路径或设备路径（实现可扩展以接受 DiskIO）
//  - err_buf:    用于接收可读错误信息的缓冲区（可为 NULL）
// 返回: 非空句柄表示成功
ntfs_parser_t ntfs_open(const char* image_path, char* err_buf, size_t err_buf_len);
void ntfs_close(ntfs_parser_t p);

// 读取下一个 MFT 条目（迭代风格）
// 返回: 0 表示成功并填充 out；非 0 表示结束或错误
int ntfs_next_record(ntfs_parser_t p, ntfs_file_record_t* out);
