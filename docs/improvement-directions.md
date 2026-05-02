# DiffSinger Dataset Tools 改进方向

> 更新时间：2026-05-02 | 详细任务见 [refactoring-roadmap.md](refactoring-roadmap.md) | 架构设计见 [task-processor-design.md](task-processor-design.md)

---

## 已完成

| 方向 | 路线图 | 优先级 |
|------|--------|--------|
| 死代码清理（11 个零消费者接口/基础设施） | G.1 | P1 |
| 任务处理器基础设施（ITaskProcessor + Registry + 数据类型） | G.2 | P1 |
| 处理器迁移（RMVPE → FunASR → HuBERT → GAME） | G.3 | P1 |
| 集成与清理（CLI/Labeler/GameInfer 切换 + 删除旧服务接口） | G.4 | P1 |
| CMake helper 函数 (`dstools_add_library` / `dstools_add_executable`) | I.1 | P1 |
| 根 CMakeLists 清理 (cmake 3.21 + qt_standard_project_setup) | I.5 | P1 |
| framework 层 CMake 迁移 (7 目标, 326→71 行) | I.2 | P1 |
| 应用层 + libs 层 CMake 迁移 (24 目标, 719→166 行) | I.3 | P1 |
| infer-target.cmake 重构 (macro→function, 消除重复 flags) | I.4 | P1 |
| 数据路径迁移 QStandardPaths（三平台路径统一） | H.5 | P1.5 |
| 统一 CrashHandler（替换 QBreakpad、修复 bug） | H.4 | P1.5 |
| 全局文件日志 Sink（FileLogSink + 7 天轮转） | H.2 | P1.5 |
| PitchLabeler 撤销重做补全（7 个 QUndoCommand） | H.1 | P1.5 |
| 批量处理 Checkpoint（BatchCheckpoint + 断点续处理） | H.3 | P1.5 |
| 窗口状态持久化 (AppShell 自动 saveGeometry/restoreGeometry) | J.1 | P1.5 |
| 单实例守卫 (SingleInstanceGuard: QLockFile + QLocalServer) | J.2 | P1.5 |
| 最近文件管理 (RecentFilesManager + QMenu 集成) | J.3 | P1.5 |
| Toast 通知 (ToastNotification 非模态浮层控件) | J.4 | P1.5 |
| 本地化基础 i18n (TranslationManager + CommonKeys::Language) | J.5 | P1.5 |
| 补齐领域模块单元测试（24 个测试模块） | B.1 | P2 |
| 框架模块独立编译 CI 验证 | D.1 | P2 |
| #pragma once 统一 (35 个头文件批量替换) | K.1 | P2 |
| 框架 Doxygen 补全 (24 个头文件) | K.2 | P2 |
| 杂项修复 (AsyncTask final, QBreakpad 注释, Result 注释) | K.3 | P2 |
| TODO/FIXME 清理 | B.2 | P3 |
| 文件操作错误处理 | B.3 | P3 |
| Doxygen CI (docs.yml) | D.2 | P3 |
| 跨平台包分发 (release.yml) | D.3 | P3 |
| 大文件拆分 (PitchLabelerPage/PhonemeLabelerPage → Setup 文件) | C.1 | P3 |
| 魔法数字常量化 (kDefaultBufferSize) | C.2 | P3 |

## 待办 — 按需拾取 (P3)

| 方向 | 路线图 | 说明 |
|------|--------|------|
| 示例项目 | F.1 | 有外部用户需求时做 |
| K.4 随修随改 | K.4 | Game snake_case、Slicer 成员变量等，触及时修复 |
