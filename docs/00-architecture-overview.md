# 架构概览

## 项目概述

DiffSinger Dataset Tools 是一套歌声合成数据集处理工具集，基于 C++17 和 Qt 6 构建。项目包含六个独立应用，覆盖从音频切片、歌词对齐到音高标注的完整工作流。

| 应用 | 功能 |
|------|------|
| **DatasetPipeline** | 统一数据集处理管线，串联 AudioSlicer、LyricFA、HubertFA 三阶段 |
| **MinLabel** | 音频标注工具，集成 G2P（Grapheme-to-Phoneme）转换 |
| **PhonemeLabeler** | TextGrid 音素边界编辑器，可视化编辑 Praat 标注文件 |
| **PitchLabeler** | DS 文件 F0 曲线编辑器，支持手动修正基频轨迹 |
| **GameInfer** | GAME 音频转 MIDI，内含 4 模型 ONNX 推理管线 |
| **DiffSingerLabeler** | 9 步数据集标注向导，引导用户完成完整标注流程 |

## 技术栈

- **语言**: C++17
- **GUI 框架**: Qt 6.8+
- **推理引擎**: ONNX Runtime（DirectML / CUDA / CPU）
- **音频解码**: FFmpeg
- **音频播放**: SDL2
- **包管理**: vcpkg
- **平台支持**: Windows（DirectML GPU 加速）、macOS、Linux

## 模块依赖图

```
┌─────────── Applications Layer ──────────────────────┐
│ DatasetPipeline  MinLabel  PhonemeLabeler            │
│ PitchLabeler     GameInfer DiffSingerLabeler        │
└──────────┬──────────────────────┬───────────────────┘
           │                      │
┌──────────▼──────────┐  ┌───────▼────────────────────┐
│ dstools-widgets     │  │ Inference Libraries         │
│ (SHARED DLL)        │  │ game-infer, some-infer,     │
│ PlayWidget,         │  │ rmvpe-infer, hubert-infer,  │
│ TaskWindow,         │  │ audio-util, FunAsr          │
│ ViewportController  │  │ (all SHARED DLLs)           │
└──────┬──────────────┘  └──────────┬──────────────────┘
       │                            │
┌──────▼──────────┐  ┌─────────────▼──────┐
│ dstools-core    │  │ dstools-infer-common │
│ (STATIC)        │  │ (STATIC)            │
│ Interfaces,     │  │ OnnxEnv singleton   │
│ Settings,       │  │ ExecutionProvider    │
│ DsDocument,     │  └────────────────────┘
│ Pipeline logic  │
└──────┬──────────┘
       │
┌──────▼──────────┐
│ dstools-audio   │  ┌────────────────┐
│ (STATIC)        │  │ textgrid (HDR) │
│ AudioDecoder,   │  │ Praat TextGrid │
│ AudioPlayback   │  │ parser/writer  │
└─────────────────┘  └────────────────┘
```

**分层说明：**

- 最上层是六个可执行应用，各自按需链接下层库
- `dstools-widgets` 提供可复用的 Qt 控件（播放器、任务窗口、视口控制器）
- 推理库群各自封装一个模型的加载与推理逻辑，均为动态链接库
- `dstools-core` 是静态核心库，定义接口协议、文档模型、管线逻辑
- `dstools-infer-common` 封装 ONNX Runtime 环境的单例管理与执行提供者抽象
- `dstools-audio` 封装音频解码（FFmpeg）和播放（SDL2），对上层隐藏实现细节
- `textgrid` 是纯头文件库，负责 Praat TextGrid 格式的解析与写入

## 核心设计模式

### Strategy / 依赖注入

项目通过接口抽象隔离具体实现，所有接口都配有 Stub 空实现，方便测试和渐进式集成：

- `IFileIOProvider` — 文件读写策略
- `IModelProvider` — 模型加载策略
- `IG2PProvider` — 字素到音素转换策略
- `IExportFormat` — 导出格式策略
- `IQualityMetrics` — 质量评估策略

### PIMPL（编译防火墙）

`AudioDecoder` 和 `AudioPlayback` 使用 PIMPL 手法，将 FFmpeg/SDL2 的头文件依赖完全隔离在 `.cpp` 中，避免污染上层编译环境。

### Singleton（单例）

- `Theme::instance()` — 全局主题管理
- `OnnxEnv::env()` — ONNX Runtime 环境，进程内唯一

### Observer（观察者）

- `AppSettings::observe()` — 配置变更通知
- `ViewportController::viewportChanged` — 视口状态变化信号

### Adapter（适配器）

`DsDocumentAdapter` 将内部文档模型 `DsDocument` 适配为通用 `IDocument` 接口，供管线和导出模块统一消费。

### Template Method（模板方法）

`TaskWindow::runTask()` 定义任务执行的骨架流程（初始化、执行、清理），子类只需覆写具体步骤。

### LRU Eviction（缓存淘汰）

`ModelManager` 管理已加载的 ONNX 模型，按 LRU 策略释放显存/内存。

### Pipeline（管线）

- `TranscriptionPipeline` — 语音转写多阶段管线
- `DsItemManager` — 数据集条目处理流水线

### Null Object（空对象）

所有 `Stub*` 类作为接口的空实现，保证系统在缺少具体实现时仍能正常运行，不会崩溃。

## 命名空间结构

```cpp
namespace dstools          // 核心类型与接口
namespace dstools::audio   // 音频解码与播放
namespace dstools::widgets // Qt 可复用控件
namespace dstools::CommonKeys // 配置项键名常量
```

## 构建系统

**基础配置：**

- CMake 3.17+，标准 CMake targets 组织
- vcpkg 管理第三方依赖（Qt、SDL2、FFmpeg 等）
- ONNX Runtime 通过专用 CMake 脚本按平台下载（DirectML / CUDA / CPU）

**部署：**

- Windows: `windeployqt` 收集 Qt 运行时
- macOS: `macdeployqt` 打包为 .app bundle

**持续集成：**

- GitHub Actions，仅 Windows 平台
- Tag 触发构建与发布
