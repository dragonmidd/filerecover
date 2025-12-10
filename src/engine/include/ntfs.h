// ntfs.h — NTFS MFT 解析骨架（最小原型）
#pragma once

#include <cstdint>
#include <cstddef>

// 不透明句柄类型，代表一个打开的 MFT 解析会话
typedef struct ntfs_parser_s* ntfs_parser_t;

typedef struct {
    uint64_t file_reference; // 文件记录引用号
    uint64_t size;
    uint64_t creation_time; // UTC FILETIME 风格（原型）
    char file_name[256];
} ntfs_file_record_t;

// 打开 MFT（接受 DiskIO 提供的镜像访问或镜像文件路径）
ntfs_parser_t ntfs_open(const char* image_path, char* err_buf, size_t err_buf_len);
void ntfs_close(ntfs_parser_t p);

// 读取下一个 MFT 条目（迭代风格），返回 0 表示成功，非 0 表示结束或错误
int ntfs_next_record(ntfs_parser_t p, ntfs_file_record_t* out);
