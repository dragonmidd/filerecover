// engine_test.cpp — GoogleTest 示例
//
// 说明：本单元测试用于验证 C API 的基本调用流（初始化、打开镜像、扫描、
// 轮询候选项与导出）。当前使用的是引擎的 stub 实现，因此测试关注点是接口契约而非恢复质量。

#include "fr.h"
#include <gtest/gtest.h>
#include <cstdlib>
#include <string>
#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

// 使用环境变量（TMP/TEMP/TMPDIR）作为临时目录，避免引入 std::filesystem
static std::string get_temp_dir() {
    const char* envs[] = { "TMPDIR", "TMP", "TEMP", nullptr };
    for (int i = 0; envs[i]; ++i) {
        const char* v = std::getenv(envs[i]);
        if (v && v[0]) return std::string(v);
    }
#if defined(_WIN32)
    return std::string(".");
#else
    return std::string("/tmp");
#endif
}

static bool ensure_dir(const std::string& path) {
#if defined(_WIN32)
    int r = _mkdir(path.c_str());
    return (r == 0 || errno == EEXIST);
#else
    int r = mkdir(path.c_str(), 0755);
    return (r == 0 || errno == EEXIST);
#endif
}

TEST(EngineStub, InitOpenScanCandidatesExport) {
    std::string tmp = get_temp_dir();
    std::string work_dir = tmp + "/filerecover_work";
    std::string out_dir = tmp + "/filerecover_out";
    ensure_dir(work_dir);
    ensure_dir(out_dir);

    // 初始化引擎（创建必要的全局资源）
    ASSERT_EQ(FR_OK, fr_init(work_dir.c_str()));

    // 打开一个假的镜像路径（stub 不验证实际文件存在）
    fr_error_t err;
    std::string fake_img = work_dir + "/fake.img";
    fr_handle_t h = fr_open_image(fake_img.c_str(), &err);
    ASSERT_NE(nullptr, h);
    ASSERT_EQ(FR_OK, err);

    // 配置扫描参数并启动扫描（stub 会填充若干候选项）
    fr_scan_params_t params;
    params.mode = FR_SCAN_QUICK;
    params.max_threads = 0; // 0 表示自动选择线程数
    ASSERT_EQ(FR_OK, fr_start_scan(h, &params));

    // 轮询并验证至少发现一个候选项，尝试导出以验证导出接口
    fr_candidate_t c;
    int found = 0;
    while (fr_get_next_candidate(h, &c) == FR_OK) {
        found++;
        // 尝试导出（stub 不写文件，仅返回 OK），实际实现应把数据写入 out 目录
        ASSERT_EQ(FR_OK, fr_export_candidate(h, c.id, out_dir.c_str()));
    }
    ASSERT_GT(found, 0);

    // 关闭句柄并释放全局资源
    fr_close(h);
    fr_shutdown();
}
