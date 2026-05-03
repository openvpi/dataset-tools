# dsfw 通用框架架构设计

## 1. 框架分离总体方案

### 1.1 目标

将项目拆分为：
- **dsfw（通用框架层）**: 可复用的 C++20/Qt6 桌面应用框架，不含歌声合成领域逻辑
- **dstools-app（应用层）**: 基于 dsfw 的 DiffSinger 数据集处理工具

### 1.2 分离原则

| 判断标准 | 归入框架 | 归入应用层 |
|---------|---------|-----------|
| 涉及 .ds/.dsproj 格式 | 否 | 是 |
| 涉及特定 G2P（拼音、假名） | 否 | 是 |
| 涉及特定 AI 模型 | 否 | 是 |
| 可脱离上述概念独立运行 | 是 | 否 |

### 1.3 总览图

```
┌─────────────────────────────────────────────────────────┐
│                  DiffSinger App Layer                    │
│  DsDocument, DsProject, PinyinG2PProvider, ExportFormats│
│  TranscriptionPipeline, CsvToDsConverter, TextGridToCsv │
│  game-infer, hubert-infer, rmvpe-infer, FunAsr          │
│  各领域 Page 实现, LabelSuite + DsLabeler + CLI + TestShell + WidgetGallery│
├─────────────────────────────────────────────────────────┤
│               dsfw (通用框架)                             │
│  types / core / audio / ui-core / widgets / infer-common│
│  AppShell ← 所有应用的统一窗口壳                          │
└─────────────────────────────────────────────────────────┘
```

---

## 2. 六层架构

```
Layer 5 ─ dsfw-infer       OnnxEnv, IInferenceEngine
Layer 4 ─ dsfw-widgets     通用 UI 组件 (SHARED DLL)
Layer 3 ─ dsfw-ui-core     AppShell, IconNavBar, Theme, FramelessHelper, IPageActions
Layer 2 ─ dsfw-audio       AudioDecoder (FFmpeg), AudioPlayback (SDL2)
Layer 1 ─ dsfw-core        AppSettings, ServiceLocator, AsyncTask, JsonHelper, 接口集
                           Logger, FileLogSink, CrashHandler, AppPaths, BatchCheckpoint
                           PipelineContext, PipelineRunner, ITaskProcessor
Layer 0.5─ dsfw-base       JsonHelper (Qt-free 静态库)
Layer 0 ─ dsfw-types       Result<T>, ExecutionProvider, TimePos (header-only)
```

### 依赖关系

```
dsfw-widgets → dsfw-ui-core → dsfw-core → dsfw-types
                                          ↑
dsfw-widgets → dsfw-audio                dsfw-infer
```

---

## 3. AppShell 统一壳

所有应用共享 `dsfw::AppShell`，不再各自实现 MainWindow。

**单页面模式**: IconNavBar 隐藏，表现为普通应用窗口
```cpp
AppShell shell("MinLabel");
shell.addPage(new MinLabelPage());
shell.show();
```

**多页面模式**: 显示侧边导航，页面切换时菜单栏/快捷键/状态栏自动跟随
```cpp
AppShell shell("DiffSinger Labeler");
shell.addPage(slicerPage, "S", "Slice");
shell.addPage(labelPage, "L", "Label");
shell.show();
```

壳与页面之间通过 `IPageLifecycle` / `IPageActions` 接口通信。

---

## 4. 松耦合策略

### 4.1 接口驱动

框架定义纯虚接口，应用层提供实现：

```
IDocument        ← DsDocumentAdapter
IModelProvider   ← GameModelProvider, HuBERTModelProvider
IG2PProvider     ← PinyinG2PProvider
IExportFormat    ← HtsLabelExportFormat, SinsyXmlExportFormat
IInferenceEngine ← GameEngine, HuBERTEngine, RmvpeEngine
```

### 4.2 ServiceLocator 依赖注入

```cpp
ServiceLocator::set<IG2PProvider>(new PinyinG2PProvider());
auto *g2p = ServiceLocator::get<IG2PProvider>();
```

### 4.3 CMake 消费

```cmake
find_package(dsfw REQUIRED)
target_link_libraries(myapp PRIVATE dsfw::core dsfw::ui-core)
```

---

## 5. 命名规范

| 模块 | CMake target | namespace | include 前缀 |
|------|-------------|-----------|-------------|
| dsfw-types | dsfw::types | dstools | `<dstools/Result.h>` |
| dsfw-base | dsfw::base | dstools | `<dsfw/JsonHelper.h>` |
| dsfw-core | dsfw::core | dstools | `<dsfw/AppSettings.h>` |
| dsfw-audio | dsfw::audio | dstools::audio | `<dstools/AudioDecoder.h>` |
| dsfw-ui-core | dsfw::ui-core | dsfw (AppShell/Theme), dstools::labeler (IPageActions/IPageLifecycle) | `<dsfw/AppShell.h>` |
| dsfw-widgets | dsfw::widgets | dsfw::widgets | `<dsfw/widgets/PlayWidget.h>` |
| dsfw-infer | dsfw::infer | dsfw::infer | — |
| dstools-domain | — | dstools | `<dstools/DsDocument.h>` |

```
框架: #include <dsfw/AppSettings.h>    namespace dstools
框架: #include <dsfw/AppShell.h>       namespace dsfw
框架: #include <dsfw/widgets/...>      namespace dsfw::widgets
应用: #include <dstools/DsDocument.h>   namespace dstools
```

---

## 6. 迁移状态

| 阶段 | 状态 |
|------|------|
| 接口泛化 (DocumentFormat/ModelType 枚举) | ✅ 已完成 (Phase 1) |
| 独立仓库 | ⏳ 待定（当前单仓库模式，ADR-8） |

---

## 7. 框架接口一览

| 接口 | 层级 | 职责 | 默认实现 |
|------|------|------|----------|
| IDocument | core | 文档生命周期 | — |
| IFileIOProvider | core | 文件 I/O | LocalFileIOProvider |
| IModelProvider | core | 模型加载/卸载 | — |
| IModelDownloader | core | 模型下载 | ModelDownloader, StubModelDownloader |
| IG2PProvider | core | G2P 转换 | StubG2PProvider |
| IExportFormat | core | 数据导出 | StubExportFormat |
| IQualityMetrics | core | 质量评估 | StubQualityMetrics |
| IAudioPlayer | audio | 音频播放 | AudioPlayer |
| IStepPlugin | ui-core | 步骤插件 | StubStepPlugin |
| IInferenceEngine | infer | 推理引擎 | — |
