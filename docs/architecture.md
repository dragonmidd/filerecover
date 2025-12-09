# 系统架构与技术选型 — FileRecover

本文档为第一版（MVP）提供高层架构、模块划分、技术栈建议与第三方依赖评估，目标是支持 Windows 10 x64，优先实现对 NTFS/exFAT/FAT 的误删文件快速恢复与深度雕刻功能，并保证易用的 GUI 和可复用的恢复引擎。

## 1 高层架构概览

组件（高层）
- 恢复引擎（Engine，C++）
  - 职责：对原始设备/镜像执行低级读取、文件系统元数据解析（NTFS/FAT/exFAT）、文件雕刻、碎片重组、哈希与完整性校验
  - 输出：发现的候选文件条目、预览数据切片、恢复数据流
  - 部署：编译为静态库/动态库（DLL）并提供 C ABI 绑定

- 平台绑定层（Bridge，C++/CLI 或 C API）
  - 职责：把 Engine 的能力以安全、稳定的 ABI 暴露给 .NET（C#）客户端；实现内存管理与错误码映射
  - 选项：优先 C API + P/Invoke（兼容性好、简单）；在需要时为高级原生调用提供 C++/CLI 包装

- 客户端（App，C# .NET 7/8）
  - 职责：GUI（WPF/WinUI）或 CLI，负责权限请求、设备/镜像选择、扫描控制、UI 交互、预览与导出
  - 部署：Windows 桌面应用；同时提供轻量 CLI（dotnet tool / exe）

- 测试与工具（tests）
  - 单元测试（Engine：GoogleTest；App：xUnit）
  - 集成测试脚本与回归测试镜像

- 打包与发布（packaging）
  - 使用 WiX / Inno Setup 生成 MSI 或 EXE 安装包；代码签名使用 Microsoft signtool

通信与数据格式
- 本地进程内调用（DLL + C ABI）。为长时任务（扫描）设计可序列化的“扫描项目文件”（JSON）以便断点续查与复现。

部署示意（运行时）
- GUI（C#） -> 调用 Bridge (C API) -> Engine (C++ DLL) -> 读取物理设备/镜像

## 2 技术选型理由（决策摘要）

- C++17/20（Engine）：
  - 原因：运行时性能与低级控制（扇区读取、内存布局、碎片重组）；生态中有成熟的原生库（zlib、libarchive、xxHash 等）

- C API 绑定 + C# (.NET 7/8) GUI：
  - 原因：.NET 提升 GUI 开发效率（WPF/WinUI），C API 通过 P/Invoke 与 .NET 高兼容性；避免过早依赖 C++/CLI 的复杂性

- CMake（C++） + dotnet SDK（C#）：
  - 原因：跨平台的构建脚本（CMake）便于在 GitHub Actions Windows runner 上构建，dotnet CLI 易于管理 .NET 项目

- vcpkg（C++ 依赖管理）：
  - 原因：在 Windows 上管理第三方 C++ 库更方便，和 CMake 集成良好

- GoogleTest（C++ 单元测试）、xUnit（C# 单元测试）：
  - 原因：行业常见，集成 CI 简单

## 3 推荐第三方库与许可考量

- 必备/优先：
  - zlib（MIT-like） — 压缩/解压支持
  - xxHash（BSD） — 快速哈希校验
  - libarchive（BSD） — 处理归档（可选）
  - libmagic/file(用于文件类型检测)（LICENCE 注意，选受欢迎的实现）

- 可选/后续：
  - sqlite（公共领域/公认）— 用于本地索引或缓存扫描结果
  - open-source carving signatures db（需评估来源许可）

- 避免或谨慎：GPL-licensed 库（若用于核心功能会影响二进制分发许可）

## 4 详细模块建议（MVP）

1) 磁盘访问层（DiskIO）
   - 功能：安全地读取物理设备或镜像（支持部分读取、跳过坏扇区）、提供缓存与并发读策略
   - Windows 实现：CreateFile/ReadFile/SetFilePointerEx 或 ReadFileScatter/ReadFileEx；使用 overlapped I/O 在必要时提升吞吐

2) 文件系统解析器（FS Parsers）
   - NTFS Parser：读取 MFT、解析索引与属性（filename, data runs）、删除标记识别
   - FAT/exFAT Parser：识别 FAT 表、目录项、长文件名记录
   - 接口：统一返回 CandidateEntry（包含偏移、长度、类型、元数据、碎片信息）

3) 文件雕刻（Carving）
   - 功能：基于签名与容错规则从原始数据流中识别文件头/尾并提取
   - 策略：签名库（可配置）+ 扩展边界检测 + 基于启发式的尾部确认

4) 碎片重组（Reassembly）
   - 功能：基于 MFT/索引和 data runs 优先组装；在缺失元数据时尝试基于相邻签名与哈希猜测重组

5) 预览与导出（Preview/Export）
   - 功能：为 GUI 提供小型数据切片与基本解码（图像缩略、文本片段）；导出时写入目标目录并生成冲突策略（重命名、覆盖、跳过）

6) 项目与状态管理（Project）
   - 功能：保存扫描配置、发现条目、扫描进度与断点信息（JSON 格式），支持导入/导出项目以便离线/跨机器续查

## 5 构建、CI 与发布流程建议

- 本地开发：
  - C++：CMake + vcpkg
  - C#：dotnet SDK（sln + csproj）

- CI（GitHub Actions）示例：
  - windows-latest：安装 vcpkg、构建 C++（CMake），构建 C#（dotnet build），运行单元测试（gtest/xunit），构建安装包（WiX），发布 artifacts

- 打包/签名：
  - 使用 WiX 生成 MSI；签名使用 Microsoft signtool（需证书与时间戳服务）

## 6 安全、权限与合规考虑

- 避免直接在原盘写入：默认不写回原盘，UI 强制建议并优先提供镜像工作流
- 若提供内核驱动以支持高级功能，必须规划 WHQL/EV 驱动签名与发布预算
- 审计日志必须可导出并提供最小化日记保留策略，避免泄露敏感数据

## 7 性能与可扩展性建议

- I/O 优化：顺序化读取优先，分块缓存，尽量减少随机读取，使用并行解析与单线程磁盘读取模型
- 内存：默认内存占用受限（可配置最大内存），大文件使用流式处理

## 8 接口契约（草案）

简单 C API（示例契约）
- int fr_init(const char* workdir, fr_config_t* cfg);
- fr_handle_t fr_open_image(const char* path);
- int fr_start_scan(fr_handle_t h, fr_scan_params_t* params, fr_callback_t cb, void* userdata);
- int fr_get_candidate(fr_handle_t h, fr_candidate_t* out);
- int fr_export_candidate(fr_handle_t h, fr_candidate_id_t id, const char* outpath);
- int fr_save_project(fr_handle_t h, const char* project_path);
- void fr_close(fr_handle_t h);

（详细接口将在详细设计阶段以头文件形式规范）

## 9 开发里程碑（短期落地计划）

- Week 0–2：架构与接口讨论，确定 C ABI，建立 CI 模板
- Week 2–8：Engine 原型（DiskIO + NTFS 基本解析 + 简单 carving），基本测试
- Week 8–12：C API 绑定 + C# GUI 原型（设备选择、快速扫描、预览、导出）
- Week 12–20：深度扫描、断点续查、碎片重组、稳定性与性能优化

## 10 风险与替代方案

- 驱动签名与内核驱动需额外预算：首版绕过，使用镜像策略
- 文件系统复杂性（Ext/HFS/特殊 RAID）：通过镜像兼容或插件式架构后续支持

---

下一步（建议）：进入“详细设计（模块与接口）”阶段：
- 我将输出模块接口说明（C API 头文件草案）、数据模型（CandidateEntry、Project JSON schema）、以及首批单元测试用例草案。是否现在开始？
