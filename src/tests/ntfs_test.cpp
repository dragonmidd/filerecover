// ntfs_test.cpp — 验证 NTFS MFT 解析骨架
#include "ntfs_mft.h"
#include "disk_io.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <chrono>
#include <string>

TEST(NTFSParser, ReadMFTRecord) {
    using namespace std::filesystem;
    auto tmpdir = temp_directory_path();
    auto suffix = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    path tmp = tmpdir / (std::string("filerecover-mft-") + suffix + std::string(".bin"));
    {
        // 写入一个带有 "FILE" 签名的 MFT-like 记录以供解析器识别
        std::ofstream of(tmp, std::ios::binary);
        std::vector<unsigned char> data(2048, 0x00);
        data[0] = 'F'; data[1] = 'I'; data[2] = 'L'; data[3] = 'E';
        // Set some header fields for testing
        data[22] = 0x01; // flags: in use
        data[18] = 1; data[19] = 0; // link_count = 1
        // attribute_offset = 48 (little-endian)
        data[20] = 48; data[21] = 0;
        // Write a resident STANDARD_INFORMATION attribute at offset 48
        size_t a = 48;
        auto write_u32 = [&](size_t off, uint32_t v){ data[off+0]=v&0xFF; data[off+1]=(v>>8)&0xFF; data[off+2]=(v>>16)&0xFF; data[off+3]=(v>>24)&0xFF; };
        auto write_u16 = [&](size_t off, uint16_t v){ data[off+0]=v&0xFF; data[off+1]=(v>>8)&0xFF; };
        auto write_u64 = [&](size_t off, uint64_t v){ for(int i=0;i<8;++i) data[off+i]= (v>>(8*i)) & 0xFF; };
        write_u32(a + 0, 0x10); // type STANDARD_INFORMATION
        write_u32(a + 4, 0x30); // length
        data[a + 8] = 0; // resident
        data[a + 9] = 0; // name length
        write_u16(a + 10, 0); // name offset
        write_u16(a + 12, 0); // flags
        write_u16(a + 14, 1); // attr id
        // resident header
        write_u32(a + 16, 16); // content size
        write_u16(a + 20, 24); // content offset (from attr start)
        data[a + 22] = 0; data[a + 23] = 0;
        // content: creation_time, modified_time
        write_u64(a + 24, 0x1122334455667788ULL);
        write_u64(a + 32, 0x99AABBCCDDEEFF00ULL);
        of.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    DiskIO d;
    ASSERT_TRUE(d.open(tmp.string().c_str()));
    NTFSParser p;
    NTFSFileRecord r;
    ASSERT_TRUE(p.read_mft_record(d, 0, r));
    EXPECT_NE(r.id, 0u);
    EXPECT_EQ(r.flags, 0x01); // Check flags parsed
    EXPECT_EQ(r.link_count, 1); // Check link_count parsed
    EXPECT_STREQ(r.name, "PARSED_FILE.TXT");
    EXPECT_GT(r.size, 0u);
    EXPECT_EQ(r.creation_time, 0x1122334455667788ULL);
    EXPECT_EQ(r.modified_time, 0x99AABBCCDDEEFF00ULL);
    d.close();
    std::error_code ec;
    remove(tmp, ec);
}
// ntfs_test.cpp — 单元测试：NTFS MFT 解析骨架
#include "ntfs.h"
#include <gtest/gtest.h>
#include <string>

TEST(NTFSStub, OpenIterateClose) {
    char err[128] = {0};
    ntfs_parser_t p = ntfs_open("/fake/image.img", err, sizeof(err));
    ASSERT_NE(nullptr, p);

    ntfs_file_record_t rec;
    int count = 0;
    while (ntfs_next_record(p, &rec) == 0) {
        ++count;
        EXPECT_GT(rec.size, 0);
    }
    EXPECT_GT(count, 0);

    ntfs_close(p);
}
