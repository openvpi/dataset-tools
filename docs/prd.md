# 产品需求文档：dsfw 通用桌面应用框架

> 从 DiffSinger Dataset Tools 提取的 C++20/Qt6 通用桌面应用框架

**版本**：Draft v0.1 | **日期**：2026-04-30 | **状态**：草案

---

## 1. 产品愿景

dsfw 是基于 C++20/Qt6 的通用桌面应用框架，为模块化桌面工具提供开箱即用的基础设施。核心理念："Page 即应用" — AppShell 负责窗口框架，Page 负责业务逻辑，同一 Page 可独立运行也可嵌入多页面壳。

## 2. 目标用户

| 角色 | 描述 |
|------|------|
| C++ 桌面开发者 | Qt6 工具/编辑器类应用 |
| 音频/ML 工具作者 | 需要音频播放、ONNX 推理 |
| 插件开发者 | 基于 IStepPlugin/IExportFormat 等扩展 |

---

## 3. 核心功能需求

### FR-01: 应用基础设施 (P0)
AppSettings（类型安全 JSON 配置）、ServiceLocator（DI 容器）、AsyncTask（异步任务）、Theme（Dark/Light 主题）

### FR-02: 文档模型 (P0)
IDocument 接口：load/save/close 生命周期、脏标记、格式识别

### FR-03: 音频播放/解码 (P1)
AudioDecoder (FFmpeg)、IAudioPlayer/AudioPlayback (SDL2)

### FR-04: 通用 UI 组件库 (P0)
AppShell（统一窗口壳）、PlayWidget、TaskWindow、ShortcutManager、GpuSelector、BaseFileListPanel、ViewportController 等

### FR-05: 无边框窗口 (P1)
FramelessHelper 封装 QWindowKit

### FR-06: ONNX 推理 (P1)
OnnxEnv 环境管理、IInferenceEngine 抽象、ExecutionProvider 枚举

### FR-07: 向导/流水线 (P1)
IStepPlugin 步骤插件、IPageActions 页面动作接口

### FR-08: 文件 I/O 抽象 (P0)
IFileIOProvider + LocalFileIOProvider，返回 Result<T>

### FR-09: JSON 工具 (P0)
JsonHelper：安全 JSON 读写、路径导航、类型安全访问

### FR-10: 模型管理 (P1)
IModelProvider + ModelManager + IModelDownloader

### FR-11: 导出格式 (P2)
IExportFormat 可扩展导出系统

### FR-12: 质量评估 (P2)
IQualityMetrics 可扩展评估系统

### FR-13: AppShell 统一壳 (P1)
单页面模式替代 MainWindow，多页面模式替代 LabelerWindow

### FR-14: 崩溃处理与日志持久化 (P1)
CrashHandler（跨平台 minidump/崩溃上下文）、FileLogSink（自动轮转文件日志）、AppPaths（跨平台数据路径管理）。AppInit 统一激活，所有应用自动受益。

### FR-15: 批量处理 Checkpoint (P1)
BatchCheckpoint 工具类：processBatch 自动记录已处理文件列表、支持断点续处理。归 dsfw-core，与 ITaskProcessor 集成。

---

## 4. 非功能需求

- **性能**: AppSettings 防抖写入、AsyncTask 线程池、AudioDecoder 流式处理
- **模块化**: 按 CMake 库拆分，可独立引用
- **可扩展**: 全部扩展点为纯虚接口 + Stub 实现
- **跨平台**: Windows 10/11 (DirectML)、macOS 11+、Linux
- **构建**: CMake >= 3.17 + vcpkg

---

## 5. 框架 vs 应用边界

**框架提供**: types, core, audio, ui-core, widgets, infer-common（通用、与业务无关）

**消费者实现**: DsDocument、具体推理引擎、具体步骤插件、具体 G2P、特定文件格式解析等

**判定原则**:
1. 两个以上应用使用或不绑定特定领域 → 框架
2. 接口归框架，具体实现归消费者
3. DiffSinger 特定格式 → 消费者
4. FFmpeg/SDL2/ORT 封装 → 框架；FunASR/wolf-midi → 消费者
5. 撤销/重做：Qt QUndoStack 直用（ADR-23），QUndoCommand 子类归消费者
6. 崩溃处理/日志/路径/Checkpoint → 框架（所有应用通过 AppInit 自动受益）

---

## 6. 成功指标

| 指标 | 目标 |
|------|------|
| 编译兼容性 | 6 个应用编译通过 |
| 外部可用性 | ≥1 个非 DiffSinger 项目成功使用 |
| 模块独立性 | 各模块可单独链接 |
| 测试覆盖 | 框架核心 ≥ 80% |
| 文档完整性 | 公开头文件 Doxygen + 使用指南 |
| 三平台 CI | Windows/macOS/Linux 全绿 |
