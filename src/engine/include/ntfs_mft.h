// ntfs_mft.h — NTFS MFT 解析骨架（最小接口）
#pragma once
#include <cstdint>
#include <cstddef>
#include "disk_io.h"

struct NTFSFileRecord {
    uint64_t id;
    char name[256];
    uint64_t size;
};

class NTFSParser {
public:
    NTFSParser();
    ~NTFSParser();

    // 从指定偏移读取并解析 MFT 记录（stub 版模拟读取）
    // 返回 true 表示解析成功并填充 out
    bool read_mft_record(DiskIO& dio, uint64_t offset, NTFSFileRecord& out);
};
