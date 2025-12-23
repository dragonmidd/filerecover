// disk_io.h — 磁盘访问抽象（轻量原型）
// 目的：为恢复引擎提供一个跨平台的磁盘/镜像读取接口。所有平台实现
// 都应提供相同语义：线程安全的随机访问读取（不改变全局文件指针）。
#pragma once
#include <cstdint>
#include <cstddef>

// DiskIO: 小而明确的类接口：打开、关闭、按偏移读取、不透明错误查询。
// 设计要点：实现细节隐藏在 impl_ 中以便在不同平台下替换实现。
struct DiskIO {
    DiskIO();
    ~DiskIO();

    // 打开镜像或设备路径（例如 "C:\\images\\disk.img" 或 "\\.\\PhysicalDrive0"）
    // 返回: true 表示成功；false 表示失败，可用 last_error() 获取可读错误信息
    bool open(const char* path);

    // 关闭此前打开的设备/镜像句柄
    void close();

    // 从指定偏移读取最多 size 字节到 buf
    // 参数:
    //  - offset: 以字节为单位的绝对偏移
    //  - buf:    目标缓冲区，调用者负责分配
    //  - size:   要读取的最大字节数
    // 返回: 实际读取的字节数；出错返回 -1
    // 语义: 实现应使用不移动全局文件指针的接口（例如 Windows 的 OVERLAPPED 或 pread），
    //       从而支持并发读取。
    ssize_t read_at(uint64_t offset, void* buf, size_t size);

    // 返回最后一次错误的可读文本（仅用于调试/日志），返回值指向内部缓冲区
    const char* last_error() const;

private:
    void* impl_; // 不透明实现指针（实现具体类型在各平台源文件中定义）
};
