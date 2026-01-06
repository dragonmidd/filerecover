// ntfs_stub.cpp — NTFS MFT 解析骨架的 stub 实现（用于原型与单元测试）
#include "ntfs.h"
#include <string>
#include <vector>
#include <cstring>

struct ntfs_parser_s {
    std::string path;
    size_t next_index = 0;
    std::vector<ntfs_file_record_t> records;
};

// 打开 NTFS 解析上下文（stub）。返回解析器句柄或 nullptr 并在 err_buf 中写入错误。
ntfs_parser_t ntfs_open(const char* image_path, char* err_buf, size_t err_buf_len) {
    if (!image_path) {
        if (err_buf && err_buf_len) strncpy(err_buf, "null path", err_buf_len-1);
        return nullptr;
    }
    ntfs_parser_s* p = new ntfs_parser_s();
    p->path = image_path;
    // 模拟填充几个记录
    for (uint64_t i = 1; i <= 3; ++i) {
        ntfs_file_record_t r;
        r.file_reference = i;
        r.size = 1024 * i;
        r.creation_time = 0;
        snprintf(r.file_name, sizeof(r.file_name), "file_%llu.txt", (unsigned long long)i);
        r.name_namespace = 0;
        p->records.push_back(r);
    }
    return p;
}

// 关闭并释放解析器句柄。
void ntfs_close(ntfs_parser_t p) {
    delete p;
}

// 获取下一个记录：返回 0 表示成功，1 表示无更多记录，负值表示参数错误。
int ntfs_next_record(ntfs_parser_t p, ntfs_file_record_t* out) {
    if (!p || !out) return -1;
    if (p->next_index >= p->records.size()) return 1; // no more
    *out = p->records[p->next_index++];
    return 0;
}

// 获取下一个记录：返回 0 表示成功，1 表示无更多记录，负值表示参数错误。
