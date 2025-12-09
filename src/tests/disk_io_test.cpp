// disk_io_test.cpp — 验证 DiskIO 抽象的最小单元测试
#include "disk_io.h"
#include <gtest/gtest.h>

TEST(DiskIOStub, OpenReadClose) {
    DiskIO d;
    // 打开一个假的路径，stub 不检查实际存在性
    ASSERT_TRUE(d.open("/nonexistent/fake.img"));

    char buf[128];
    ssize_t n = d.read_at(0, buf, sizeof(buf));
    ASSERT_GT(n, 0);
    // stub 填充 0xAA
    EXPECT_EQ((unsigned char)buf[0], 0xAA);

    d.close();
    // 关闭后应返回错误
    n = d.read_at(0, buf, sizeof(buf));
    ASSERT_EQ(n, -1);
}
