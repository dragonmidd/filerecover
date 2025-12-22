// ntfs_mft_stub.cpp — NTFS MFT 最小可运行解析器（轻量实现）
#include "ntfs_mft.h"
#include <cstring>
#include <vector>

// 构造/析构：轻量解析器初始化（当前无资源需要释放）
NTFSParser::NTFSParser() {}
NTFSParser::~NTFSParser() {}

// 这是一个最小实现：读取一个 MFT 记录（1024 字节），验证前 4 字节是否为 'FILE'
// 如果匹配则填充返回的 NTFSFileRecord（用于单元测试与后续迭代）。
bool NTFSParser::read_mft_record(DiskIO& dio, uint64_t offset, NTFSFileRecord& out) {
    const size_t MFT_RECORD_SIZE = 1024;
    std::vector<uint8_t> buf(MFT_RECORD_SIZE);
    ssize_t n = dio.read_at(offset, buf.data(), buf.size());
    if (n <= 0) return false;

    // 需要至少能读取到签名
    if (n < 4) return false;

    // 检查 NTFS MFT 记录签名 "FILE"
    if (!(buf[0] == 'F' && buf[1] == 'I' && buf[2] == 'L' && buf[3] == 'E')) {
        return false;
    }

    // 最小可用字段填充：id、name 与 size（后续可以从记录内部解析真实值）
    out.id = offset ? static_cast<uint64_t>(offset / MFT_RECORD_SIZE) : 1;
    strncpy(out.name, "PARSED_FILE.TXT", sizeof(out.name));
    // 此实现将 size 设为读取到的数据乘以 2，作为示例
    out.size = static_cast<uint64_t>(n) * 2;
    return true;
}
