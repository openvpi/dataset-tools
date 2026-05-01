# DiffSinger Dataset Tools — Issue 清单

> 最后更新：2026-05-02

---

## 开放 Issue

| Issue # | 标题 | 优先级 | 状态 | 路线图 |
|---------|------|--------|------|--------|
| — | PitchLabeler 撤销重做补全 | P1.5 | 📋 待执行 | H.1 |
| — | 全局文件日志 Sink | P1.5 | 📋 待执行 | H.2 |
| — | 批量处理 Checkpoint | P1.5 | 📋 待执行 | H.3 |
| #39 | 大文件拆分 | P3 | 📋 按需 | C.1 |
| #40 | 魔法数字 | P3 | 📋 按需 | C.2 |

**已关闭**: #11 (领域测试 — 24 个测试模块已覆盖), #15 (框架独立编译 — CI 已验证), #16 (API 文档 CI — docs.yml 已配置), #18 (DLL 导出审查), #19 (labeler-interfaces), #21 (CI 矩阵), #27 (clang-tidy), #28 (TranscriptionPipeline 可测试性 — Deps 注入+测试已完成), #37 (Slicer), #38 (MinLabel)

---

### H.1: PitchLabeler 撤销重做补全

已有 2 个 QUndoCommand：`PitchMoveCommand`（音高拖动）、`ModulationDriftCommand`（颤音/偏移 F0）。以下 5 个编辑操作仍直接修改 DSFile 绕过 QUndoStack：

| 操作 | 信号来源 | 需要新建的 Command |
|------|---------|-------------------|
| 删除音符 | `noteDeleteRequested` | `DeleteNotesCommand` |
| 修改滑音类型 | `noteGlideChanged` | `SetNoteGlideCommand` |
| 切换连音标记 | `noteSlurToggled` | `ToggleNoteSlurCommand` |
| 切换休止符 | `noteRestToggled` | `ToggleNoteRestCommand` |
| 合并到左邻音符 | `noteMergeLeft` | `MergeNoteLeftCommand` |

`PianoRollInputHandler.cpp:305` 和 `:385` 也有直接 `markModified()` 调用待审计。详见 refactoring-roadmap.md Phase H.1。

### H.2: 全局文件日志 Sink

Logger 有 pluggable sink 但未注册文件 sink。需在 dsfw-core 新增 `FileLogSink`，由 AppInit 注册，写入 `AppPaths::logDir()`，支持 7 天自动轮转。详见 refactoring-roadmap.md Phase H.2。

### H.3: 批量处理 Checkpoint

BatchOutput 仅记录计数，无具体文件列表。需新增 BatchCheckpoint 工具类，支持断点续处理。详见 refactoring-roadmap.md Phase H.3。

### #39: 大文件拆分

PitchLabelerPage (755行)、PhonemeLabelerPage (600行) 有维护痛点时拆分。

### #40: 魔法数字

`4096` buffer size 3 处重复（WaveformRenderer.cpp:29, WaveformWidget.cpp:62, AudioFileLoader.cpp:61），优先提取。其余按需。

---

## 已完成 Issue 详情

### #11: 领域模块单元测试 — ✅ 已完成

24 个测试模块（domain 10 + framework 12 + libs 1 + widgets 1），80+ 用例。原待测的 CsvToDsConverter、TextGridToCsv、PitchProcessor、TranscriptionPipeline 均已补齐。

### #15: 框架模块独立编译 — ✅ 已完成

dsfw-core / dsfw-ui-core 有 `find_package` guards，`.github/workflows/verify-modules.yml` CI 已创建。

### #16: API 文档 CI — ✅ 已完成

32 个框架头文件（core + ui-core + base）已有 Doxygen 注释（仅 AppPaths.h 待补）。`.github/workflows/docs.yml` 已配置。

### #28: TranscriptionPipeline 可测试性 — ✅ 已完成

Deps 注入、步骤拆分、StringUtils 提取均已完成。`TestTranscriptionPipeline.cpp` 已编写。

### H.4: 统一 CrashHandler — ✅ 已完成

QBreakpad 已移除。CrashHandler 已重写，使用 `AppPaths::dumpDir()`。注意：`AppInit.h:22` 仍有 QBreakpad 注释残留待清理。

### H.5: 数据路径迁移 QStandardPaths — ✅ 已完成

`AppPaths` 工具类已创建，使用 `QStandardPaths::AppDataLocation`。config/logs/dumps 路径已迁移，支持 `migrateFromLegacyPaths()` 旧路径自动迁移。`applicationDirPath()` 仅剩用于 model/dict 等捆绑资源路径（合理）。
