// disk_io.h — 磁盘访问抽象（轻量原型）
// 目的：为恢复引擎提供一个最小的磁盘访问层接口，便于在不同平台上替换实现。
#pragma once
#include <cstdint>
#include <cstddef>

struct DiskIO {
    DiskIO();
    ~DiskIO();

    // 打开镜像或设备路径（如 \\\\.\\PhysicalDrive0 或 镜像文件路径）
    // 返回 true 表示成功，否则返回 false 并可通过 last_error() 查询字符串
    bool open(const char* path);
    void close();

    // 从指定偏移读取最多 size 字节到 buf，返回实际读取的字节数，出错返回 -1
    // 实现应保证不会移动文件指针（以便多线程安全的读取策略）
    ssize_t read_at(uint64_t offset, void* buf, size_t size);

    // 最后一次错误文本（只用于调试/测试原型）
    const char* last_error() const;

private:
    void* impl_; // 不透明实现指针
};
