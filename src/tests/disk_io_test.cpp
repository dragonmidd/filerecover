// disk_io_test.cpp — 验证 DiskIO 抽象的最小单元测试
#include "disk_io.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <vector>
#include <chrono>
#include <string>

TEST(DiskIOStub, OpenReadClose) {
    using namespace std::filesystem;
    // 创建一个临时文件，写入已知数据供原生实现打开并读取
    auto tmpdir = temp_directory_path();
    auto suffix = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    path tmp = tmpdir / (std::string("filerecover-") + suffix + std::string(".bin"));
    {
        std::ofstream of(tmp, std::ios::binary);
        std::vector<unsigned char> data(1024, 0xAA);
        of.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    DiskIO d;
    ASSERT_TRUE(d.open(tmp.string().c_str()));

    char buf[128];
    ssize_t n = d.read_at(0, buf, sizeof(buf));
    ASSERT_GT(n, 0);
    EXPECT_EQ(static_cast<unsigned char>(buf[0]), 0xAA);

    d.close();
    // 关闭后应返回错误
    n = d.read_at(0, buf, sizeof(buf));
    ASSERT_EQ(n, -1);

    // 清理临时文件
    std::error_code ec;
    remove(tmp, ec);
}
