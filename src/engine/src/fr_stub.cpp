// fr_stub.cpp — 简单的引擎 stub 实现，用于本地开发与测试
//
// 说明：此实现为占位（stub），用于在早期开发阶段验证 C API、构建和单元测试流程。
// 真正的引擎实现会替换这些函数，增加磁盘 I/O、文件系统解析与雕刻逻辑。
#include "fr.h"
#include <string>
#include <mutex>
#include <vector>
#include <atomic>
#include <cstring>

// 内部句柄结构：持有会话上下文（打开的镜像路径、扫描状态及已发现候选项）

struct fr_handle_s {
    std::string path;                 // 打开的镜像或设备路径
    std::atomic<bool> scanning{false}; // 当前是否处于扫描状态
    std::mutex m;                      // 保护 candidates 与 next_index 的互斥量
    std::vector<fr_candidate_t> candidates; // 已发现的候选文件列表（简化）
    size_t next_index{0};              // 下一个轮询索引
};

// 全局初始化标志：fr_init / fr_shutdown 控制生命周期
static bool g_inited = false;

// 初始化引擎（例如设置工作目录、日志系统、全局资源）
fr_error_t fr_init(const char* workdir) {
    (void)workdir; // stub 不使用 workdir，但真实实现可在此创建缓存/日志目录
    g_inited = true;
    return FR_OK;
}

// 释放全局资源
void fr_shutdown(void) {
    g_inited = false;
}

// 打开镜像或设备，返回一个句柄用于后续操作。error 可选，用于返回详细错误码。
fr_handle_t fr_open_image(const char* path, fr_error_t* err) {
    if (!g_inited) {
        if (err) *err = FR_ERR_NOT_INITIALIZED;
        return nullptr;
    }
    if (!path) {
        if (err) *err = FR_ERR_INVALID_ARG;
        return nullptr;
    }
    fr_handle_s* h = new fr_handle_s();
    h->path = path;
    if (err) *err = FR_OK;
    return h;
}

// 关闭并释放句柄及其关联资源
void fr_close(fr_handle_t h) {
    if (!h) return;
    delete h;
}

// 开始扫描：stub 实现只是模拟发现若干候选文件。
// 真实实现应触发异步扫描/多线程读取、解析并将结果入队。
fr_error_t fr_start_scan(fr_handle_t h, const fr_scan_params_t* params) {
    if (!h) return FR_ERR_INVALID_ARG;
    // 简单模拟：填充若干候选项以供轮询测试使用
    std::lock_guard<std::mutex> lk(h->m);
    h->candidates.clear();
    for (uint64_t i = 1; i <= 5; ++i) {
        fr_candidate_t c;
        c.id = i;
        c.offset = 512 * i;
        c.size = 1024 * i;
        snprintf(c.file_name, sizeof(c.file_name), "recovered_%llu.jpg", (unsigned long long)i);
        strncpy(c.mime_type, "image/jpeg", sizeof(c.mime_type));
        h->candidates.push_back(c);
    }
    h->next_index = 0;
    h->scanning.store(true);
    (void)params; // params 在 stub 中未使用
    return FR_OK;
}

// 轮询获取下一个候选项：非阻塞风格。真实实现可改为回调或事件驱动。
fr_error_t fr_get_next_candidate(fr_handle_t h, fr_candidate_t* out) {
    if (!h || !out) return FR_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lk(h->m);
    if (h->next_index >= h->candidates.size()) return FR_ERR_NOT_FOUND;
    *out = h->candidates[h->next_index++];
    return FR_OK;
}

// 导出候选项到指定路径：stub 仅验证候选项存在并返回 OK。
// 真正实现应创建目录、写入字节流并处理冲突策略。
fr_error_t fr_export_candidate(fr_handle_t h, uint64_t candidate_id, const char* out_path) {
    if (!h || !out_path) return FR_ERR_INVALID_ARG;
    // stub: 不实际写文件，仅验证存在
    std::lock_guard<std::mutex> lk(h->m);
    for (auto &c : h->candidates) {
        if (c.id == candidate_id) return FR_OK;
    }
    return FR_ERR_NOT_FOUND;
}

// 保存/加载扫描项目（JSON）：stub 不做实际存储。
fr_error_t fr_save_project(fr_handle_t h, const char* project_path) {
    (void)h; (void)project_path;
    return FR_OK;
}

fr_error_t fr_load_project(fr_handle_t h, const char* project_path) {
    (void)h; (void)project_path;
    return FR_OK;
}
