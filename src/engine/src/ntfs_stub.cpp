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
        p->records.push_back(r);
    }
    return p;
}

void ntfs_close(ntfs_parser_t p) {
    delete p;
}

int ntfs_next_record(ntfs_parser_t p, ntfs_file_record_t* out) {
    if (!p || !out) return -1;
    if (p->next_index >= p->records.size()) return 1; // no more
    *out = p->records[p->next_index++];
    return 0;
}
