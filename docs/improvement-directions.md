# DiffSinger Dataset Tools 改进方向

> 更新时间：2026-05-01 | 详细任务见 [refactoring-roadmap.md](refactoring-roadmap.md) | 架构设计见 [task-processor-design.md](task-processor-design.md)

---

## P1 — 架构演进（核心价值）

| 方向 | 路线图 | 状态 |
|------|--------|------|
| 死代码清理（11 个零消费者接口/基础设施） | G.1 | ✅ 已完成 |
| 任务处理器基础设施（ITaskProcessor + Registry + 数据类型） | G.2 | ✅ 已完成 |
| 处理器迁移（RMVPE → FunASR → HuBERT → GAME） | G.3 | ✅ 已完成 |
| 集成与清理（CLI/Labeler/GameInfer 切换 + 删除旧服务接口） | G.4 | ✅ 已完成 |

## P1.5 — 用户体验与可靠性

| 方向 | 路线图 | 状态 |
|------|--------|------|
| 数据路径迁移 QStandardPaths（三平台路径统一） | H.5 | 📋 待执行 |
| 统一 CrashHandler（替换 QBreakpad、修复 4 个 bug） | H.4 | 📋 待执行 |
| 全局文件日志 Sink（崩溃前日志持久化） | H.2 | 📋 待执行 |
| PitchLabeler 撤销重做补全（5+ 个操作绕过 UndoStack） | H.1 | 📋 待执行 |
| 批量处理 Checkpoint（已处理文件记录 + 断点续处理） | H.3 | 📋 待执行 |

## P2 — 有实际价值

| 方向 | 路线图 | 状态 |
|------|--------|------|
| 补齐领域模块单元测试 | B.1 | ⏳ 4 个模块待测 |
| 框架模块独立编译 CI 验证 | D.1 | ✅ 已完成 |

## P3 — 按需拾取

| 方向 | 路线图 | 说明 |
|------|--------|------|
| TODO/FIXME 清理 | B.2 | 5 处，随手修 |
| 文件操作错误处理 | B.3 | 补 else 分支 |
| 大文件拆分 | C.1 | PitchLabelerPage (781行), PhonemeLabelerPage (630行) |
| 魔法数字常量化 | C.2 | `4096` 3 处重复 |
| Doxygen CI | D.2 | 头文件注释已就绪 |
| 跨平台包分发 | D.3 | ZIP/DMG/AppImage |
| 示例项目 | F.1 | 有外部用户需求时做 |
