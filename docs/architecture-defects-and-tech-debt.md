# 架构缺陷与技术债务清单

> 最后更新：2026-05-01

---

## 技术债

| 编号 | 描述 | 严重性 | 路线图 |
|------|------|--------|--------|
| TD-03 | 4 个领域模块缺单元测试 (CsvToDsConverter, TextGridToCsv, PitchProcessor, TranscriptionPipeline) | 中 | B.1 |
| TD-04 | 部分 `file.open()` 缺错误分支 | 低 | B.3 |
| TD-05 | PitchLabelerPage (781行), PhonemeLabelerPage (630行) 职责过多 | 低 | C.1 |
| TD-06 | `4096` buffer size 3 处重复 | 低 | C.2 |
| TD-07 | 5 处 TODO/FIXME | 低 | B.2 |
| TD-15 | PitchLabeler 5+ 个编辑操作（删除/滑音/连音/休止/合并 + PianoRollInputHandler 中的直接修改）绕过 QUndoStack，不可撤销 | 中 | H.1 |
| TD-16 | 所有应用无持久化日志文件，崩溃后只有 minidump 无上下文 | 中 | H.2 |
| TD-17 | 批量处理无 checkpoint 机制，中断后必须从头重新处理 | 中 | H.3 |
| TD-18 | config/logs/dumps 路径基于 `applicationDirPath()`，macOS .app bundle 和 Linux /usr/bin 安装场景不可写 | 中 | H.5 |
| TD-19 | dsfw::CrashHandler 是死代码（从未 `install()`），且有 4 个 bug：`m_callback` 未调用、Unix 无 dump 写入、缺 `#include <csignal>`、与 QBreakpad 双机制并存互不感知 | 高 | H.4 |
