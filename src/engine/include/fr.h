#pragma once
/*
 fr.h — 简化的 C API 头文件（FileRecover 引擎）

 目的：为引擎提供稳定的 C ABI，使得其他语言（例如 C#/.NET）可以通过
 P/Invoke 或相似机制调用引擎功能。接口保持最小且明确，便于逐步扩展。
*/
#ifndef FR_H
#define FR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// 简短错误码：调用方根据需要可以将其扩展为更精细的错误体系。
typedef enum {
    FR_OK = 0,
    FR_ERR_GENERIC = 1,
    FR_ERR_IO = 2,
    FR_ERR_INVALID_ARG = 3,
    FR_ERR_NOT_FOUND = 4,
    FR_ERR_NOT_INITIALIZED = 5
} fr_error_t;

// 不透明句柄：代表一个打开的镜像/设备会话。客户端代码不应直接解引用。
typedef struct fr_handle_s* fr_handle_t;

// 扫描模式：快速或深度扫描。
typedef enum {
    FR_SCAN_QUICK = 0, // 更快、依赖文件系统元数据或简单签名
    FR_SCAN_DEEP = 1   // 更全面，可能包含数据雕刻等耗时操作
} fr_scan_mode_t;

// 扫描参数：目前只包含扫描模式与线程数量（0 表示自动决定）。
typedef struct {
    fr_scan_mode_t mode;
    uint32_t max_threads; // 0 = 自动选择（基于硬件）
} fr_scan_params_t;

// 扫描候选项：向上层报告发现的可恢复文件信息。
// 说明：为保持 ABI 简洁，使用固定缓冲区传递字符串（调用方应处理长度限制）。
typedef struct {
    uint64_t id;            // 引擎内部分配的候选项标识
    uint64_t offset;        // 在镜像/设备上的偏移（字节）
    uint64_t size;          // 估计大小（字节）
    char file_name[256];    // 文件名（若可推断）
    char mime_type[64];     // MIME 类型提示（可选）
} fr_candidate_t;

// 初始化（必须在调用其他 fr API 之前调用）
// 参数: workdir - 引擎用于临时文件与项目保存的工作目录
// 返回: FR_OK 或错误码
fr_error_t fr_init(const char* workdir);

// 释放初始化资源
void fr_shutdown(void);

// 打开镜像或设备
// 参数: path - 镜像文件路径或物理设备路径（例如 "\\.\\PhysicalDrive0"）
//        err  - 可选输出错误码指针（NULL 表示忽略）
// 返回: 非 NULL 的 fr_handle_t 表示成功
fr_handle_t fr_open_image(const char* path, fr_error_t* err);
void fr_close(fr_handle_t h);

// 启动扫描（同步接口）
// 参数: h      - 已打开的句柄
//        params - 指定扫描模式与线程数（NULL 可使用默认值）
// 返回: FR_OK 或错误码
fr_error_t fr_start_scan(fr_handle_t h, const fr_scan_params_t* params);

// 获取下一个候选项（轮询）
// 参数: h   - 会话句柄
//        out - 输出缓冲区，调用者负责分配
// 返回: FR_OK 表示成功并填充 out；FR_ERR_NOT_FOUND 表示当前无更多候选项
fr_error_t fr_get_next_candidate(fr_handle_t h, fr_candidate_t* out);

// 导出候选项到指定路径（目录或文件）
// 参数: candidate_id - 候选项 id
//        out_path      - 输出路径
// 返回: FR_OK 或错误码
fr_error_t fr_export_candidate(fr_handle_t h, uint64_t candidate_id, const char* out_path);

// 保存/加载项目（用于断点续查）
fr_error_t fr_save_project(fr_handle_t h, const char* project_path);
fr_error_t fr_load_project(fr_handle_t h, const char* project_path);

#ifdef __cplusplus
}
#endif

#endif // FR_H
