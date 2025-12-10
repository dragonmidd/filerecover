// ntfs_test.cpp — 验证 NTFS MFT 解析骨架
#include "ntfs_mft.h"
#include "disk_io.h"
#include <gtest/gtest.h>

TEST(NTFSParser, ReadMFTRecord) {
    DiskIO d;
    ASSERT_TRUE(d.open("/fake.img"));
    NTFSParser p;
    NTFSFileRecord r;
    ASSERT_TRUE(p.read_mft_record(d, 0, r));
    EXPECT_NE(r.id, 0u);
    EXPECT_STREQ(r.name, "FILE_STUB.TXT");
    EXPECT_GT(r.size, 0u);
    d.close();
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
