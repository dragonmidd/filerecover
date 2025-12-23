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
