// ntfs_mft.h — NTFS MFT 解析骨架（最小接口）
#pragma once
#include <cstdint>
#include <cstddef>
#include "disk_io.h"

// NTFS MFT Record Header (simplified, based on NTFS spec)
struct MFTHeader {
    char signature[4];              // "FILE"
    uint16_t usa_offset;            // Update Sequence Array offset
    uint16_t usa_size;              // Update Sequence Array size
    uint64_t lsn;                   // Log File Sequence Number
    uint16_t sequence_number;       // Sequence number
    uint16_t link_count;            // Hard link count
    uint16_t attribute_offset;      // First attribute offset
    uint16_t flags;                 // Flags (e.g., 0x01 = in use, 0x02 = directory)
    uint32_t record_size;           // Real size of record
    uint32_t allocated_size;        // Allocated size of record
    uint64_t base_record;           // Base record (for extension records)
    uint16_t next_attr_id;          // Next attribute ID
};

// NTFS 文件记录（简化表示）。实际实现会从 MFT 记录中提取
// 文件标识、文件名、大小以及其他属性。
struct NTFSFileRecord {
    uint64_t id;        // 记录标识（例如 MFT 记录号）
    char name[256];     // 文件名（如果可用）
    uint64_t size;      // 文件大小（字节）
    // 新增字段用于 header 解析
    uint16_t flags;     // MFT record flags
    uint16_t link_count; // Hard link count
    // STANDARD_INFORMATION fields
    uint64_t creation_time; // FILETIME (100-ns intervals since 1601)
    uint64_t modified_time;
    uint64_t parent_reference; // parent directory file reference (from FILE_NAME)
};

// NTFSParser: 提供从镜像/设备读取并解析 MFT 记录的最小接口。
// 说明:
//  - read_mft_record 会从指定偏移读取并解析单个 MFT 记录；
//  - 若解析成功则返回 true 并填充 out；否则返回 false。
class NTFSParser {
public:
    NTFSParser();
    ~NTFSParser();

    // 参数:
    //  - dio:    平台无关的磁盘读取接口（DiskIO）
    //  - offset: MFT 记录在镜像中的字节偏移
    //  - out:    输出解析结果
    // 返回: true 表示成功并填充 out；false 表示解析失败或数据不完整
    bool read_mft_record(DiskIO& dio, uint64_t offset, NTFSFileRecord& out);

private:
    // 解析 MFT record header
    // 返回: true if header is valid and parsed
    bool parse_header(const uint8_t* data, size_t size, MFTHeader& header);
};
