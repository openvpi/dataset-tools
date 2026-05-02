# 架构重构路线图

> 基于 2026-05-02 代码审计更新。
>
> **原则**: 功能齐全、简单可靠、不过度设计。接口保持一致性和合理的扩展预留。
>
> **范围**: 在本仓库内完成。所有模块作为 CMake 子目录共存。
>
> **C++ 标准**: 允许 C++20（当前 C++20，已有 `std::filesystem` 使用）。

---

## 已完成阶段摘要

| Phase | 名称 | 主要成果 |
|-------|------|---------|
| 0 | 预备工作 | ModelType/DocumentFormat 泛化，Qt 依赖分离，pipeline 子模块库化 |
| 1 | 核心分离 | ModelManager→domain，dsfw-base 创建，OnnxModelBase，IInferenceEngine |
| 2 | 库边界固化 | 目录重排，版本化 API (DSFW_VERSION)，FunASR 适配器，pipeline 后端提取 |
| 3 | 框架增强 | Logger，Undo/Redo (ICommand+UndoStack)，EventBus，CLI 工具 (dstools-cli) |
| 4 | 完善与扩展 | PluginManager，CrashHandler，UpdateChecker，RecentFiles，WidgetGallery，新标准控件 |
| 5-6 | 深度优化 | Result\<T\> 统一，UI 推理解耦，Slicer 合并，MinLabelService 提取，StringUtils 提取，CI 矩阵构建，clang-tidy CI，CHANGELOG，线程安全验证，冗余依赖清理 |
| B | 测试与质量 | 24 个测试模块 (80+ 用例)，TODO/FIXME 清理，文件操作错误处理补全 |
| C | 代码质量 | PitchLabelerPage/PhonemeLabelerPage 大文件拆分 (→Setup 文件)，魔法数字常量化 (kDefaultBufferSize) |
| D | CI/CD | 框架独立编译 CI (verify-modules.yml)，Doxygen CI (docs.yml)，跨平台包分发 (release.yml) |
| G | 任务处理器架构 | 死代码清理 (11 个)，ITaskProcessor + Registry，4 个处理器迁移，CLI/Labeler/GameInfer 集成，旧服务接口删除 |
| H | 用户体验与可靠性 | AppPaths (QStandardPaths)，CrashHandler 统一 (替换 QBreakpad)，FileLogSink (7 天轮转)，PitchLabeler 撤销重做补全 (7 个 Command)，BatchCheckpoint (断点续处理) |
| I | CMake 现代化 | DstoolsHelpers.cmake (dstools_add_library/dstools_add_executable)，40+ CMakeLists.txt 迁移 (1045→237 行)，cmake 3.21，qt_standard_project_setup，infer-target.cmake macro→function 重构 |
| J | 框架功能补全 | 窗口状态持久化 (AppShell)，SingleInstanceGuard，RecentFilesManager，ToastNotification，TranslationManager (i18n) |
| K | 代码规范化 | 35 个头文件 #pragma once 统一，24 个框架头文件 Doxygen 补全，AsyncTask/AppInit/Result 杂项修复 |

---

## 架构决策记录 (ADR)

| ADR | 决策 | 理由 |
|-----|------|------|
| ADR-1 | dsfw-base 为 Qt-free 静态库 | 支持 CLI 工具和非 Qt 消费者 |
| ADR-2 | ModelType 改为 int ID 注册表 | 框架层不感知业务模型类型 |
| ADR-3 | OnnxModelBase protected 继承 | 各推理 DLL 保持现有 API 稳定 |
| ADR-4 | audio-util 保持应用层 | 音频 DSP 过于专业化 |
| ADR-5 | dsfw-audio (FFmpeg/SDL2) 归框架 | 音频播放/解码是通用桌面能力 |
| ADR-6 | Undo/Redo 迁移 opt-in | 现有 app 按各自节奏迁移 |
| ADR-7 | FunASR 永远不直接修改 | vendor 代码只用适配器包装 |
| ADR-8 | 单仓库模式 | 降低维护复杂度 |
| ADR-9 | 允许 C++20 | 编译器均已支持，按需使用新特性，不强制全面迁移 |
| ADR-21 | 替换 QBreakpad，统一用 dsfw::CrashHandler | QBreakpad 为外部依赖且 dsfw::CrashHandler 已存在但未接入；修复后统一使用，减少依赖 |
| ADR-22 | config/logs/dumps 迁移到 QStandardPaths::AppDataLocation | applicationDirPath() 在 macOS .app bundle 和 Linux /usr/bin 安装下可能只读 |
| ADR-23 | 撤销重做直接使用 QUndoStack，不封装框架抽象 | Qt QUndoStack 已成熟，旧 dsfw::UndoStack 零消费者已删除；QUndoCommand 子类是纯业务逻辑无法泛化 |
| ADR-24 | CMake 源文件采用 `file(GLOB_RECURSE CONFIGURE_DEPENDS)` | 项目已用 AUTOMOC（隐式扫描文件），GLOB+CONFIGURE_DEPENDS 实际风险极低；通过 helper 函数封装，未来可切换策略 |
| ADR-25 | 推理库（infer/）保留独立命名风格 | audio-util/game-infer/FunAsr 为半外部推理库，API 面向领域用户；强制改名收益低于破坏成本 |

---

## 待办 — 随修随改项 (不列入计划，触及时修复)

| 项目 | 说明 |
|------|------|
| Game 类 snake_case/camelCase 混用 | `load_model()`/`get_midi()` 等，修改时统一为 camelCase |
| Slicer 成员变量 snake_case | `sample_rate`/`hop_size` 等，修改时加 `m_` 前缀 |
| FunAsr `create_model()` 返回裸指针 | ADR-7 禁止直接修改 vendor 代码，在调用处用 `std::unique_ptr` 包装 |
| paraformer_onnx.h 命名空间注释 | `} // namespace paraformer` — vendor 代码，不修改 (ADR-7) |

---

## 待办 — 按需改进 (P3, 有需求时再做)

### F.1 示例项目 — P3, M (2-8h)

在 `examples/` 创建最小非 DiffSinger 应用，演示 dsfw 框架独立使用。当有外部用户需要参考时再做。

---

## 剩余技术债

| 编号 | 描述 | 严重性 |
|------|------|--------|
| TD-04 | 少数 UI 便捷路径的 file.open() 缺 else 分支 | 低 |

> 更新时间：2026-05-02

---

## 关联 Issue

所有计划内 Issue 已关闭。

已关闭: #11 (领域测试), #15 (框架独立编译), #16 (API 文档 CI), #18 (DLL 导出), #19 (labeler-interfaces), #21 (CI 矩阵), #27 (clang-tidy), #28 (TranscriptionPipeline), #37 (Slicer), #38 (MinLabel), #39 (大文件拆分), #40 (魔法数字), CMake 现代化 (I.1-I.5), 框架功能补全 (J.1-J.5), 代码规范化 (K.1-K.3)
