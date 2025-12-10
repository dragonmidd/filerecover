// ntfs_mft_stub.cpp — NTFS MFT 解析骨架的 stub 实现
#include "ntfs_mft.h"
#include <cstring>

NTFSParser::NTFSParser() {}
NTFSParser::~NTFSParser() {}

bool NTFSParser::read_mft_record(DiskIO& dio, uint64_t offset, NTFSFileRecord& out) {
    // 简单 stub: 尝试读取 512 字节并返回伪造的记录
    uint8_t buf[512];
    ssize_t n = dio.read_at(offset, buf, sizeof(buf));
    if (n <= 0) return false;
    out.id = offset ? (offset / 1024) : 1;
    strncpy(out.name, "FILE_STUB.TXT", sizeof(out.name));
    out.size = 1024;
    return true;
}
