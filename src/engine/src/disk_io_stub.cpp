// disk_io_stub.cpp — 磁盘访问抽象的 stub 实现（便于单元测试）
#include "disk_io.h"
#include <string>
#include <cstring>
#include <mutex>

struct DiskIOImpl {
    std::string last_err;
    bool opened = false;
    std::string path;
    std::mutex m;
};

// 构造/析构：管理 stub 实现的生命周期
DiskIO::DiskIO() : impl_(new DiskIOImpl()) {}

DiskIO::~DiskIO() { delete static_cast<DiskIOImpl*>(impl_); impl_ = nullptr; }

// 打开 stub 路径：在测试中标记为已打开并记录路径。线程安全。
bool DiskIO::open(const char* path) {
    if (!impl_) return false;
    DiskIOImpl* d = static_cast<DiskIOImpl*>(impl_);
    std::lock_guard<std::mutex> lk(d->m);
    if (!path) { d->last_err = "null path"; return false; }
    d->path = path;
    d->opened = true;
    d->last_err.clear();
    return true;
}

// 关闭 stub（将 opened 标志重置）。线程安全。
void DiskIO::close() {
    if (!impl_) return;
    DiskIOImpl* d = static_cast<DiskIOImpl*>(impl_);
    std::lock_guard<std::mutex> lk(d->m);
    d->opened = false;
}

// 模拟读取：用重复字节填充缓冲区以便测试消费。返回填充字节数或错误码。
ssize_t DiskIO::read_at(uint64_t /*offset*/, void* buf, size_t size) {
    if (!impl_) return -1;
    DiskIOImpl* d = static_cast<DiskIOImpl*>(impl_);
    std::lock_guard<std::mutex> lk(d->m);
    if (!d->opened) { d->last_err = "not opened"; return -1; }
    // stub: 用固定字节填充缓冲区以模拟读取
    if (buf && size > 0) {
        memset(buf, 0xAA, size > 256 ? 256 : size);
        return static_cast<ssize_t>(size);
    }
    return 0;
}

// 返回最近一次操作的错误信息字符串，指向内部存储。
const char* DiskIO::last_error() const {
    if (!impl_) return "no impl";
    DiskIOImpl* d = static_cast<DiskIOImpl*>(impl_);
    return d->last_err.c_str();
}

// 返回最近一次操作的错误信息字符串，指向内部存储。
