# DiffSinger Dataset Tools — Issue 清单

> 最后更新：2026-05-01

---

## 开放 Issue

| Issue # | 标题 | 优先级 | 状态 | 路线图 |
|---------|------|--------|------|--------|
| #11 | 领域模块单元测试 | P2 | ⏳ 部分完成 | B.1 |
| #15 | 框架模块独立编译 | P2 | ✅ 已完成 | D.1 |
| #16 | API 文档 CI | P3 | ⏳ CI 未完成 | D.2 |
| #28 | TranscriptionPipeline 可测试性 | P2 | ⏳ 障碍已清除，待写测试 | B.1 |
| #39 | 大文件拆分 | P3 | 📋 按需 | C.1 |
| #40 | 魔法数字 | P3 | 📋 按需 | C.2 |
| — | PitchLabeler 撤销重做补全 | P1.5 | 📋 待执行 | H.1 |
| — | 全局文件日志 Sink | P1.5 | 📋 待执行 | H.2 |
| — | 批量处理 Checkpoint | P1.5 | 📋 待执行 | H.3 |
| — | 统一 CrashHandler（替换 QBreakpad） | P1.5 | 📋 待执行 | H.4 |
| — | 数据路径迁移 QStandardPaths | P1.5 | 📋 待执行 | H.5 |

**已关闭**: #18 (DLL 导出审查 — 已修复 GpuInfo，CI 矩阵构建已覆盖三平台验证), #19 (labeler-interfaces — 无外部消费者，暂不需要版本策略), #21 (CI 矩阵), #27 (clang-tidy), #37 (Slicer), #38 (MinLabel)

---

### #11: 领域模块单元测试

已完成 6 个测试模块 (TestDsDocument, TestF0Curve, TestPitchUtils, TestPhNumCalculator, TestTranscriptionCsv, TestStringUtils, TestMinLabelService)。

待完成: CsvToDsConverter, TextGridToCsv, PitchProcessor, TranscriptionPipeline 集成测试。

### #15: 框架模块独立编译

dsfw-core / dsfw-ui-core 已有 `find_package` guards。缺 `.github/workflows/verify-modules.yml` CI 验证。

### #16: API 文档 CI

23 个框架头文件已有 Doxygen 注释，Doxyfile 已配置。缺 CI 自动生成 + GitHub Pages 发布。

### #28: TranscriptionPipeline 可测试性

Deps 注入、步骤拆分、StringUtils 提取均已完成。待编写集成测试用例。

### #39: 大文件拆分

PitchLabelerPage (781行)、PhonemeLabelerPage (630行) 有维护痛点时拆分。

### #40: 魔法数字

`4096` buffer size 3 处重复，优先提取。其余按需。

### H.1: PitchLabeler 撤销重做补全

5 个编辑操作（noteDelete, noteGlideChanged, noteSlurToggled, noteRestToggled, noteMergeLeft）直接修改 DSFile 绕过 QUndoStack。需为每个操作创建 QUndoCommand 子类。详见 refactoring-roadmap.md Phase H.1。

### H.2: 全局文件日志 Sink

Logger 有 pluggable sink 但未注册文件 sink。需在 AppInit 中添加 FileLogSink，写入 `<app_dir>/logs/`，支持 7 天自动轮转。详见 refactoring-roadmap.md Phase H.2。

### H.3: 批量处理 Checkpoint

BatchOutput 仅记录计数，无具体文件列表。需新增 BatchCheckpoint 工具类，在 `dstemp/` 下写入 checkpoint JSON，支持断点续处理。详见 refactoring-roadmap.md Phase H.3。

### H.4: 统一 CrashHandler（替换 QBreakpad）

代码审计发现 dsfw::CrashHandler 是死代码（从未被 `install()`），与 QBreakpad 双机制并存。CrashHandler 自身有 4 个 bug：`m_callback` 未调用、Unix 无 dump 写入、缺 `#include <csignal>`、Windows filter 未调用回调。决定修复 CrashHandler 后替换 QBreakpad，减少外部依赖。详见 refactoring-roadmap.md Phase H.4。

### H.5: 数据路径迁移 QStandardPaths

所有应用的 config/dumps 路径基于 `applicationDirPath()`，在 macOS .app bundle（Contents/MacOS/ 不可写）和 Linux /usr/bin 安装场景失败。需迁移到 `QStandardPaths::AppDataLocation`，新增 `AppPaths` 工具类统一管理，并支持旧路径自动迁移。详见 refactoring-roadmap.md Phase H.5。
