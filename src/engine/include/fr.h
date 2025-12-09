#pragma once
// fr.h — 简化的 C API 头文件草案，用于 FileRecover 恢复引擎
//
// 目的：为恢复引擎提供稳定且轻量的 C ABI，以便从其他语言（例如 C#/.NET）
// 调用。此文件仅为初始草案，会在详细设计阶段扩展为更完整的接口。
#ifndef FR_H
#define FR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// 错误码：简单的返回码枚举。调用方应根据需要扩展或引入详细错误结构。
typedef enum {
    FR_OK = 0,
    FR_ERR_GENERIC = 1,
    FR_ERR_IO = 2,
    FR_ERR_INVALID_ARG = 3,
    FR_ERR_NOT_FOUND = 4,
    FR_ERR_NOT_INITIALIZED = 5
} fr_error_t;

// 不透明句柄类型：代表一个打开的镜像/设备实例。调用方通过此句柄
// 对特定会话进行扫描、导出等操作。
typedef struct fr_handle_s* fr_handle_t;

// 扫描模式：快速扫描使用元数据/记录优先，深度扫描使用文件雕刻等更慢但更全面的策略。
typedef enum {
    FR_SCAN_QUICK = 0,
    FR_SCAN_DEEP = 1
} fr_scan_mode_t;

// 扫描参数：用于配置扫描行为（例如并行线程数和扫描模式）。
typedef struct {
    fr_scan_mode_t mode;
    uint32_t max_threads; // 0 = 自动
} fr_scan_params_t;

// 候选文件条目（简化）：用于在扫描过程中向上层返回发现的可恢复文件信息。
// 注意：file_name 与 mime_type 使用固定长度缓冲区以便在 C ABI 中安全传递。
typedef struct {
    uint64_t id;
    uint64_t offset;
    uint64_t size;
    char file_name[256];
    char mime_type[64];
} fr_candidate_t;

// 初始化/反初始化：应在任何 fr_* API 调用之前/之后调用。
fr_error_t fr_init(const char* workdir);
void fr_shutdown(void);

// 打开镜像或设备：path 可以是镜像文件路径（例如 .dd/.img/.vhdx）或物理设备
// 路径（例如 \\.\\PhysicalDrive0）。返回非空句柄表示成功，错误通过 err 返回。
fr_handle_t fr_open_image(const char* path, fr_error_t* err);
void fr_close(fr_handle_t h);

// 启动扫描：开始对已打开的句柄执行扫描。当前接口为简单同步启动实现，
// 实际引擎可以实现异步回调或事件机制。
fr_error_t fr_start_scan(fr_handle_t h, const fr_scan_params_t* params);

// 获取下一个候选项：轮询方式从内部队列读取下一个发现的候选文件。
// 若没有更多候选项返回 FR_ERR_NOT_FOUND。
fr_error_t fr_get_next_candidate(fr_handle_t h, fr_candidate_t* out);

// 导出候选项：将指定候选项导出到 out_path（文件或目录）。具体行为（覆盖/重命名）
// 由实现决定或另行指定策略参数。
fr_error_t fr_export_candidate(fr_handle_t h, uint64_t candidate_id, const char* out_path);

// 保存/加载项目：用于持久化扫描项目以支持断点续查与离线分析（JSON 格式）。
fr_error_t fr_save_project(fr_handle_t h, const char* project_path);
fr_error_t fr_load_project(fr_handle_t h, const char* project_path);

#ifdef __cplusplus
}
#endif

#endif // FR_H
