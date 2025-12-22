// disk_io_win.cpp — Windows native DiskIO implementation
#include "../include/disk_io.h"
#include <windows.h>
#include <string>
#include <algorithm>

struct DiskIO_Impl {
    HANDLE h = INVALID_HANDLE_VALUE;
    std::string last_err;
};

// 构造函数：初始化底层实现指针
DiskIO::DiskIO() : impl_(new DiskIO_Impl()) {}

// 析构函数：确保句柄关闭并释放实现对象
DiskIO::~DiskIO() {
    close();
    delete static_cast<DiskIO_Impl*>(impl_);
    impl_ = nullptr;
}

// 打开指定路径（文件或设备）用于只读访问。
// 成功返回 true；失败时可通过 `last_error()` 获取详细信息。
bool DiskIO::open(const char* path) {
    DiskIO_Impl* p = static_cast<DiskIO_Impl*>(impl_);
    if (!p) return false;
    p->h = CreateFileA(path,
                       GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       nullptr,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       nullptr);
    if (p->h == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        char buf[512] = {0};
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, (DWORD)sizeof(buf), nullptr);
        p->last_err = buf;
        return false;
    }
    p->last_err.clear();
    return true;
}

// 关闭当前已打开的句柄（如果有）。此操作幂等。
void DiskIO::close() {
    DiskIO_Impl* p = static_cast<DiskIO_Impl*>(impl_);
    if (!p) return;
    if (p->h != INVALID_HANDLE_VALUE) {
        CloseHandle(p->h);
        p->h = INVALID_HANDLE_VALUE;
    }
}

// 从指定偏移量读取 `size` 字节到 `buf`。
// 返回实际读取的字节数，出错返回 -1，遇到 EOF 返回已读取的较小字节数。
ssize_t DiskIO::read_at(uint64_t offset, void* buf, size_t size) {
    DiskIO_Impl* p = static_cast<DiskIO_Impl*>(impl_);
    if (!p || p->h == INVALID_HANDLE_VALUE) return -1;
    // ReadFile with OVERLAPPED to avoid moving file pointer
    // Windows ReadFile takes a DWORD for size; if caller requests > 0xFFFFFFFF, read in chunks.
    const size_t MAX_CHUNK = 0xFFFFFFFFu;
    size_t remaining = size;
    uint8_t* out = static_cast<uint8_t*>(buf);
    size_t totalRead = 0;

    while (remaining > 0) {
        DWORD toRead = (DWORD)std::min(remaining, MAX_CHUNK);
        OVERLAPPED ov = {};
        ov.Offset = (DWORD)(offset & 0xFFFFFFFF);
        ov.OffsetHigh = (DWORD)((offset >> 32) & 0xFFFFFFFF);
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(p->h, out + totalRead, toRead, &bytesRead, &ov);
        if (!ok) {
            DWORD err = GetLastError();
            // If we get ERROR_HANDLE_EOF and bytesRead==0, treat as EOF
            if (err == ERROR_HANDLE_EOF) {
                break;
            }
            char bufErr[512] = {0};
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                           nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), bufErr, (DWORD)sizeof(bufErr), nullptr);
            p->last_err = bufErr;
            return -1;
        }
        totalRead += bytesRead;
        if (bytesRead < toRead) break; // EOF
        remaining -= bytesRead;
        offset += bytesRead;
    }
    return (ssize_t)totalRead;
}

// 返回最后一次操作的错误描述，返回值指向内部缓冲区，调用者无需释放。
const char* DiskIO::last_error() const {
    DiskIO_Impl* p = static_cast<DiskIO_Impl*>(impl_);
    if (!p) return "not-initialized";
    return p->last_err.c_str();
}

// 返回最后一次操作的错误描述，返回值指向内部缓冲区，调用者无需释放。
