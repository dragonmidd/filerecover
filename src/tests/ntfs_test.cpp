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

        // Write a resident FILE_NAME attribute at offset 128
        size_t f = 128;
        write_u32(f + 0, 0x30); // FILE_NAME
        write_u32(f + 4, 0x50); // length
        data[f + 8] = 0; // resident
        write_u32(f + 16, 0x24); // content size 36+name
        write_u16(f + 20, 24); // content offset
        // content
        write_u64(f + 24 + 0, 0xCAFEBABE01234567ULL); // parent reference
        // skip times
        write_u64(f + 24 + 40, 12345ULL); // allocated_size placeholder
        write_u64(f + 24 + 48, 123ULL); // real_size placeholder
        // flags/reparse
        write_u16(f + 24 + 64, 0); // (name length at offset 64 below)
        // name length and ns
        const char* fname = "sample.txt";
        size_t fnlen = strlen(fname);
        data[f + 24 + 64] = static_cast<uint8_t>(fnlen);
        data[f + 24 + 65] = 0; // namespace
        // write UTF-16LE name starting at f+24+66
        size_t name_pos = f + 24 + 66;
        for (size_t i = 0; i < fnlen; ++i) {
            data[name_pos + i*2] = fname[i];
            data[name_pos + i*2 + 1] = 0;
        }
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

TEST(NTFSParser, FileNameUtf16) {
    using namespace std::filesystem;
    auto tmpdir = temp_directory_path();
    auto suffix = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    path tmp = tmpdir / (std::string("filerecover-mft-utf16-") + suffix + std::string(".bin"));
    {
        std::ofstream of(tmp, std::ios::binary);
        std::vector<unsigned char> data(2048, 0x00);
        data[0] = 'F'; data[1] = 'I'; data[2] = 'L'; data[3] = 'E';
        data[20] = 48; data[21] = 0; // attribute_offset
        size_t f = 48;
        auto write_u32 = [&](size_t off, uint32_t v){ data[off+0]=v&0xFF; data[off+1]=(v>>8)&0xFF; data[off+2]=(v>>16)&0xFF; data[off+3]=(v>>24)&0xFF; };
        auto write_u16 = [&](size_t off, uint16_t v){ data[off+0]=v&0xFF; data[off+1]=(v>>8)&0xFF; };
        auto write_u64 = [&](size_t off, uint64_t v){ for(int i=0;i<8;++i) data[off+i]= (v>>(8*i)) & 0xFF; };

        // FILE_NAME resident attribute at offset 48
        write_u32(f + 0, 0x30); // FILE_NAME
        write_u32(f + 4, 0x50); // length
        data[f + 8] = 0; // resident
        // UTF-16LE encoding for "文件.txt": U+6587 U+4EF6 '.' 't' 'x' 't'
        unsigned char utf16le_name[] = { 0x87,0x65, 0xF6,0x4E, 0x2E,0x00, 0x74,0x00, 0x78,0x00, 0x74,0x00 };
        size_t name_bytes = sizeof(utf16le_name);
        write_u32(f + 16, static_cast<uint32_t>(66 + name_bytes)); // content size
        write_u16(f + 20, 24); // content offset
        // content: parent ref + times omitted; name length at content+64
        write_u64(f + 24 + 0, 0x1111222233334444ULL);
        // set name length for 6 characters: 文 件 . t x t
        data[f + 24 + 64] = 6; // name length (characters)
        data[f + 24 + 65] = 0; // namespace
        size_t name_pos = f + 24 + 66;
        for (size_t i = 0; i < sizeof(utf16le_name); ++i) data[name_pos + i] = utf16le_name[i];
        of.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    DiskIO d;
    ASSERT_TRUE(d.open(tmp.string().c_str()));
    NTFSParser p;
    NTFSFileRecord r;
    ASSERT_TRUE(p.read_mft_record(d, 0, r));
    // Expect the UTF-8 decoded name equals the UTF-8 string literal
    EXPECT_STREQ(r.name, u8"文件.txt");
    d.close();
    std::error_code ec;
    remove(tmp, ec);
}

TEST(NTFSParser, NonResidentDataRuns) {
    using namespace std::filesystem;
    auto tmpdir = temp_directory_path();
    auto suffix = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    path tmp = tmpdir / (std::string("filerecover-mft-nr-") + suffix + std::string(".bin"));
    {
        std::ofstream of(tmp, std::ios::binary);
        std::vector<unsigned char> data(2048, 0x00);
        data[0] = 'F'; data[1] = 'I'; data[2] = 'L'; data[3] = 'E';
        // attribute offset
        data[20] = 48; data[21] = 0;
        size_t a = 48;
        auto write_u32 = [&](size_t off, uint32_t v){ data[off+0]=v&0xFF; data[off+1]=(v>>8)&0xFF; data[off+2]=(v>>16)&0xFF; data[off+3]=(v>>24)&0xFF; };
        auto write_u16 = [&](size_t off, uint16_t v){ data[off+0]=v&0xFF; data[off+1]=(v>>8)&0xFF; };
        auto write_u64 = [&](size_t off, uint64_t v){ for(int i=0;i<8;++i) data[off+i]= (v>>(8*i)) & 0xFF; };

        // DATA attribute (non-resident) at offset a
        write_u32(a + 0, 0x80); // DATA
        // we'll set total attr length later; choose 128
        write_u32(a + 4, 128);
        data[a + 8] = 1; // non-resident
        // runlist offset at +32 (we will place runlist at a + 56)
        write_u16(a + 32, 56);
        // real size at +48 (8 bytes)
        write_u64(a + 48, 0x0000000000003039ULL); // 12345

        // build a simple runlist at a+56: one run with length=2 clusters, lcn delta=5
        size_t rl = a + 56;
        // header: len_size=1, off_size=3 -> 0x31
        data[rl + 0] = 0x31;
        data[rl + 1] = 0x02; // cluster_count = 2
        data[rl + 2] = 0x05; data[rl + 3] = 0x00; data[rl + 4] = 0x00; // lcn delta = 5
        data[rl + 5] = 0x00; // terminator

        of.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    DiskIO d;
    ASSERT_TRUE(d.open(tmp.string().c_str()));
    NTFSParser p;
    NTFSFileRecord r;
    ASSERT_TRUE(p.read_mft_record(d, 0, r));
    // Expect parsed size
    EXPECT_EQ(r.size, 12345ULL);
    // Expect one data run with count 2 and lcn 5
    ASSERT_GT(r.data_runs.size(), 0u);
    EXPECT_EQ(r.data_runs[0].first, 2ULL);
    EXPECT_EQ(r.data_runs[0].second, 5LL);
    d.close();
    std::error_code ec;
    remove(tmp, ec);
}

TEST(NTFSParser, MapFileRange) {
    NTFSFileRecord rec;
    rec.data_runs.clear();
    // two runs: 2 clusters at LCN 5, then 3 clusters at LCN 10
    rec.data_runs.emplace_back(2ULL, 5LL);
    rec.data_runs.emplace_back(3ULL, 10LL);

    NTFSParser p;
    std::vector<std::pair<uint64_t,size_t>> out;
    // cluster size 512
    bool ok = p.map_file_range(rec, 0, 512 * 5, 512, out);
    ASSERT_TRUE(ok);
    // Expect mappings covering 5 clusters: first 2 from LCN 5, next 3 from LCN 10
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0].first, 5ULL * 512ULL);
    EXPECT_EQ(out[0].second, 2ULL * 512ULL);
    EXPECT_EQ(out[1].first, 10ULL * 512ULL);
    EXPECT_EQ(out[1].second, 3ULL * 512ULL);

    // Partial read starting inside first run (spans first and second run)
    out.clear();
    ok = p.map_file_range(rec, 512, 1024, 512, out); // start at 1 cluster, length 2 clusters
    ASSERT_TRUE(ok);
        ASSERT_EQ(out.size(), 2u);
        EXPECT_EQ(out[0].first, 5ULL * 512ULL + 512ULL);
        EXPECT_EQ(out[0].second, 512ULL);
        EXPECT_EQ(out[1].first, 10ULL * 512ULL);
        EXPECT_EQ(out[1].second, 512ULL);
    EXPECT_EQ(out[0].first, 5ULL * 512ULL + 512ULL);
    EXPECT_EQ(out[0].second, 512ULL);
    EXPECT_EQ(out[1].first, 10ULL * 512ULL);
    EXPECT_EQ(out[1].second, 512ULL);
}

TEST(NTFSParser, ReadFileRangeNonResident) {
    using namespace std::filesystem;
    auto tmpdir = temp_directory_path();
    auto suffix = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    path tmp = tmpdir / (std::string("filerecover-mft-read-") + suffix + std::string(".bin"));
    const std::string payload = "RECOVER_ME_12345";
    const uint64_t lcn = 512; // choose LCN beyond MFT area so payload isn't overwritten
    {
        std::ofstream of(tmp, std::ios::binary);
        std::vector<unsigned char> data(2048, 0x00);
        // write payload at disk offset lcn
        for (size_t i = 0; i < payload.size(); ++i) data[lcn + i] = static_cast<unsigned char>(payload[i]);

        // MFT record header at 0
        data[0] = 'F'; data[1] = 'I'; data[2] = 'L'; data[3] = 'E';
        data[20] = 48; data[21] = 0; // attribute_offset
        size_t a = 48;
        auto write_u32 = [&](size_t off, uint32_t v){ data[off+0]=v&0xFF; data[off+1]=(v>>8)&0xFF; data[off+2]=(v>>16)&0xFF; data[off+3]=(v>>24)&0xFF; };
        auto write_u16 = [&](size_t off, uint16_t v){ data[off+0]=v&0xFF; data[off+1]=(v>>8)&0xFF; };
        auto write_u64 = [&](size_t off, uint64_t v){ for(int i=0;i<8;++i) data[off+i]= (v>>(8*i)) & 0xFF; };

        // DATA non-resident attribute
        write_u32(a + 0, 0x80);
        write_u32(a + 4, 128);
        data[a + 8] = 1; // non-resident
        // runlist offset at +32 (we place runlist at a+56)
        write_u16(a + 32, 56);
        write_u64(a + 48, static_cast<uint64_t>(payload.size())); // real size

        size_t rl = a + 56;
        // header: len_size=1, off_size=2 -> 0x21
        data[rl + 0] = 0x21;
        data[rl + 1] = static_cast<unsigned char>(payload.size() & 0xFF); // cluster_count = payload.size() (cluster_size==1)
        // lcn delta = lcn (100)
        data[rl + 2] = static_cast<unsigned char>(lcn & 0xFF);
        data[rl + 3] = static_cast<unsigned char>((lcn >> 8) & 0xFF);
        data[rl + 4] = 0x00; // terminator

        of.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    DiskIO d;
    ASSERT_TRUE(d.open(tmp.string().c_str()));
    NTFSParser p;
    NTFSFileRecord r;
    ASSERT_TRUE(p.read_mft_record(d, 0, r));
    ASSERT_GT(r.data_runs.size(), 0u);
    EXPECT_EQ(r.data_runs[0].second, static_cast<int64_t>(lcn));

    std::vector<uint8_t> buf(payload.size());
    ASSERT_TRUE(p.read_file_range(d, r, 0, payload.size(), buf.data(), buf.size(), 1));
    std::string got(reinterpret_cast<char*>(buf.data()), buf.size());
    EXPECT_EQ(got, payload);

    d.close();
    std::error_code ec;
    remove(tmp, ec);
}

TEST(NTFSParser, ReadFileRangeFromRuns) {
    using namespace std::filesystem;
    auto tmpdir = temp_directory_path();
    auto suffix = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    path tmp = tmpdir / (std::string("filerecover-data-") + suffix + std::string(".bin"));
    const uint32_t bpc = 512; // small cluster size for test convenience
    const uint64_t lcn_start = 5;
    const uint64_t write_offset = lcn_start * bpc;
    const std::string payload = "HELLOWORLD";
    {
        std::ofstream of(tmp, std::ios::binary);
        std::vector<uint8_t> data(write_offset + 4096, 0);
        // Place payload at write_offset
        for (size_t i = 0; i < payload.size(); ++i) data[write_offset + i] = static_cast<uint8_t>(payload[i]);
        of.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    DiskIO d;
    ASSERT_TRUE(d.open(tmp.string().c_str()));
    NTFSParser p;
    NTFSFileRecord rec{};
    rec.size = write_offset + payload.size();
    rec.data_runs.clear();
    rec.data_runs.emplace_back(2ULL, static_cast<int64_t>(lcn_start)); // 2 clusters starting at lcn 5

    std::vector<uint8_t> out;
    // The file's logical offset 0 maps to disk LCN 'lcn_start'. Read from logical offset 0.
    ASSERT_TRUE(p.read_file_range(d, rec, 0, payload.size(), out, bpc));
    ASSERT_EQ(out.size(), payload.size());
    std::string got(out.begin(), out.end());
    EXPECT_EQ(got, payload);

    d.close();
    std::error_code ec;
    remove(tmp, ec);
}

TEST(NTFSParser, ComplexDataRunsWithSparseAndNegativeDelta) {
    using namespace std::filesystem;
    auto tmpdir = temp_directory_path();
    auto suffix = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    path tmp = tmpdir / (std::string("filerecover-complex-runs-") + suffix + std::string(".bin"));

    // We'll use cluster_size == 1 for simplicity so LCN == disk offset
    // Runlist layout (logical file order):
    // run1: 2 clusters @ LCN=5 -> disk offsets 5,6 (fill with 'A')
    // run2: 3 clusters @ LCN=10 -> disk offsets 10,11,12 (fill with 'B')
    // run3: 1 sparse cluster -> logical position zero-filled
    // run4: 2 clusters @ LCN=8 (negative delta from previous 10 -> -2) -> disk offsets 8,9 (fill with 'D')
    const std::string expected = "AABBB\0DD";
    const uint64_t max_disk = 1024; // ensure space for full MFT record + runlist + payload
    {
        std::ofstream of(tmp, std::ios::binary);
        std::vector<uint8_t> data(max_disk, 0);
        // place payloads at their disk offsets
        data[5] = 'A'; data[6] = 'A';
        data[10] = 'B'; data[11] = 'B'; data[12] = 'B';
        data[8] = 'D'; data[9] = 'D';

        // build MFT-like record at start with non-resident DATA attribute and runlist
        data[0] = 'F'; data[1] = 'I'; data[2] = 'L'; data[3] = 'E';
        // attribute offset
        data[20] = 48; data[21] = 0;
        size_t a = 48;
        auto write_u32 = [&](size_t off, uint32_t v){ data[off+0]=v&0xFF; data[off+1]=(v>>8)&0xFF; data[off+2]=(v>>16)&0xFF; data[off+3]=(v>>24)&0xFF; };
        auto write_u16 = [&](size_t off, uint16_t v){ data[off+0]=v&0xFF; data[off+1]=(v>>8)&0xFF; };
        auto write_u64 = [&](size_t off, uint64_t v){ for(int i=0;i<8;++i) data[off+i]= (v>>(8*i)) & 0xFF; };

        write_u32(a + 0, 0x80); // DATA
        write_u32(a + 4, 128);
        data[a + 8] = 1; // non-resident
        // runlist offset at +32 (we place runlist at a+56)
        write_u16(a + 32, 56);
        // real size = total clusters = 2+3+1+2 = 8
        write_u64(a + 48, 8ULL);

        size_t rl = a + 56;
        // run1: header len=1 off=1 -> 0x11, count=2, delta=5
        data[rl + 0] = 0x11; data[rl + 1] = 0x02; data[rl + 2] = 0x05;
        // run2: header 0x11, count=3, delta=5 (prev 5 -> 10)
        data[rl + 3] = 0x11; data[rl + 4] = 0x03; data[rl + 5] = 0x05;
        // run3: sparse run header len=1 off=0 -> 0x01, count=1
        data[rl + 6] = 0x01; data[rl + 7] = 0x01;
        // run4: header 0x11, count=2, delta = -2 -> 0xFE
        data[rl + 8] = 0x11; data[rl + 9] = 0x02; data[rl +10] = 0xFE;
        // terminator
        data[rl + 11] = 0x00;

        of.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    DiskIO d;
    ASSERT_TRUE(d.open(tmp.string().c_str()));
    NTFSParser p;
    NTFSFileRecord r;
    ASSERT_TRUE(p.read_mft_record(d, 0, r));
    // Expect four runs
    ASSERT_EQ(r.data_runs.size(), 4u);
    EXPECT_EQ(r.data_runs[0].first, 2ULL);
    EXPECT_EQ(r.data_runs[0].second, 5LL);
    EXPECT_EQ(r.data_runs[1].first, 3ULL);
    EXPECT_EQ(r.data_runs[1].second, 10LL);
    EXPECT_EQ(r.data_runs[2].first, 1ULL);
    EXPECT_EQ(r.data_runs[2].second, -1LL);
    EXPECT_EQ(r.data_runs[3].first, 2ULL);
    EXPECT_EQ(r.data_runs[3].second, 8LL);

    // Now read the full logical file of 8 bytes and verify sparse region yields zero
    std::vector<uint8_t> out;
    ASSERT_TRUE(p.read_file_range(d, r, 0, 8, out, 1));
    ASSERT_EQ(out.size(), 8u);
    std::string got(out.begin(), out.end());
    // Build expected string with embedded zero
    std::string expect;
    expect.push_back('A'); expect.push_back('A'); // run1
    expect.push_back('B'); expect.push_back('B'); expect.push_back('B'); // run2
    expect.push_back('\0'); // sparse
    expect.push_back('D'); expect.push_back('D'); // run4
    EXPECT_EQ(got, expect);

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
