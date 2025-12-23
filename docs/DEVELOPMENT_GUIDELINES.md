# 开发指南（必须遵守的两条原则）

本项目要求所有开发成员严格遵守下面两条必须贯彻的原则，任何代码变更与 PR 都应满足这两条要求：

1) **所有关键源码都要加上简洁易读的注释。**
   - 何为“关键源码”：包含公有 API、模块边界、复杂算法、平台/系统交互、错误/异常处理逻辑、并发/同步代码等。
   - 注释风格与要求（简洁、说明意图）：
     - 在头文件（`.h` / `.hpp`）对导出的函数/类/结构体说明用途、重要参数含义、返回值与错误条件。
     - 在实现文件（`.cpp` / `.c`）对复杂逻辑块加入短注释，说明为何这样实现（why > how）。
     - 注释不应重复代码本身（避免“注释即代码”的冗余），应侧重设计意图、边界条件与不显而易见的假设。
   - 示例（函数注释示例）：
     ```cpp
     // Reads `size` bytes at `offset` from the open device/image.
     // Returns number of bytes read or -1 on error (check last_error()).
     ssize_t read_at(uint64_t offset, void* buf, size_t size);
     ```

2) **每天结束工作时要提醒并保存当天 chat 日志，并将工程上传到 GitHub。**
   - 具体流程（手动或使用脚本）：
     1. 在 `docs/work_sessions/` 下创建或更新当天日志文件，命名约定为 `YYYY-MM-DD-chat-log.md`（或使用 `2025-12-22-chat-log.md` 样式）。
     2. 确认本地变更已被 commit（合理的 commit message，并把未跟踪文件纳入版本控制或明确排除）。
     3. 将变更 push 到远程主仓库：`git push origin <branch>`。
   - 注意事项：在推送前请确保不包含敏感信息（API keys、密码、私有凭证）；若日志包含此类信息，应先移除或转为私有存储。

---

自动化支持
- 仓库中包含 `scripts/end_of_day.ps1`（Windows PowerShell），用于在当天结束时提示、打开日志文件并可交互式执行 commit/push。你可以在 VS Code 中通过自定义任务执行该脚本（仓库已包含 `.vscode/tasks.json`）。

代码审查与合并策略
- 所有 PR 在合并前应验证：注释是否充分、重要代码是否有单元测试、日志已更新（若当天有重大设计变更），以及 CI 构建通过。

变更本指南
- 若需调整上述原则或自动化流程，请创建 PR 并在 PR 描述中说明原因与变更影响，等待团队审查通过后合并。
