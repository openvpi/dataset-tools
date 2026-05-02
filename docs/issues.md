# DiffSinger Dataset Tools — Issue 清单

> 最后更新：2026-05-02

---

## 开放 Issue

| Issue # | 标题 | 优先级 | 状态 | 路线图 |
|---------|------|--------|------|--------|
| — | K.4 随修随改 | P2 | 📋 触及时修复 | K.4 |

---

## 已关闭 Issue

**全部关闭**: #11 (领域测试), #15 (框架独立编译), #16 (API 文档 CI), #18 (DLL 导出审查), #19 (labeler-interfaces), #21 (CI 矩阵), #27 (clang-tidy), #28 (TranscriptionPipeline 可测试性), #37 (Slicer), #38 (MinLabel)

| Issue # | 标题 | 完成摘要 |
|---------|------|---------|
| — | CMake 现代化 (I.1-I.5) | DstoolsHelpers.cmake helper 函数，40+ CMakeLists.txt 迁移 (1045→237 行)，cmake 3.21，qt_standard_project_setup |
| — | 窗口状态持久化 (J.1) | AppShell 自动 saveGeometry/restoreGeometry via AppSettings，移除 PhonemeLabeler 手动代码 |
| — | 单实例守卫 (J.2) | SingleInstanceGuard (QLockFile + QLocalServer)，server name 含用户名 |
| — | 最近文件管理 (J.3) | RecentFilesManager + QMenu 集成，pipe-delimited 持久化，可配上限 |
| — | Toast 通知 (J.4) | ToastNotification 非模态浮层控件，slide-in/fade-out 动画，三种样式，堆叠显示 |
| — | 本地化基础 i18n (J.5) | TranslationManager + CommonKeys::Language，AppInit 自动加载系统 locale |
| — | 代码规范化 (K.1-K.3) | 35 个头文件 #pragma once，24 个头文件 Doxygen，AsyncTask/AppInit/Result 杂项修复 |
| #39 | 大文件拆分 (C.1) | PitchLabelerPage (870→358+529)，PhonemeLabelerPage (715→422+317) |
| #40 | 魔法数字 (C.2) | WaveformRenderer/WaveformWidget kDefaultBufferSize 常量化 |
| #11 | 领域模块单元测试 | 24 个测试模块 (domain 10 + framework 12 + libs 1 + widgets 1)，80+ 用例 |
| #15 | 框架模块独立编译 | find_package guards + verify-modules.yml CI |
| #16 | API 文档 CI | 32 个框架头文件 Doxygen 注释 + docs.yml CI |
| #28 | TranscriptionPipeline 可测试性 | Deps 注入 + TestTranscriptionPipeline.cpp |
| — | 任务处理器架构 (G.1-G.4) | ITaskProcessor + Registry，4 处理器迁移，旧服务接口删除 |
| — | CrashHandler 统一 (H.4) | QBreakpad 移除，CrashHandler 重写，AppPaths::dumpDir() |
| — | 数据路径迁移 (H.5) | AppPaths + QStandardPaths + migrateFromLegacyPaths |
| — | 全局文件日志 (H.2) | FileLogSink + 7 天自动轮转 |
| — | 撤销重做补全 (H.1) | 7 个 QUndoCommand (PitchMove, ModulationDrift, DeleteNotes, SetNoteGlide, ToggleNoteSlur, ToggleNoteRest, MergeNoteLeft) |
| — | 批量 Checkpoint (H.3) | BatchCheckpoint 工具类 + processBatch 集成 |
| — | TODO/FIXME 清理 (B.2) | 应用代码 TODO 全部清理 |
| — | 文件操作错误处理 (B.3) | file.open() else 分支补全 |
| — | Doxygen CI (D.2) | docs.yml 已配置 |
| — | 跨平台包分发 (D.3) | release.yml (ZIP/DMG/AppImage) |
