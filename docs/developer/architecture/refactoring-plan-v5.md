# Dataset Tools 重构方案 v5

> 版本 5.0 | 2026-06-03
>
> v5 基于 v4 全面代码二次核实后重写。核实发现：Phase F（图表绘制架构统一）已完成但 v4 误标记为"待开始"；D-02（test-shell 迁移）已完成但 v4 在 §4 中误列为待开始。
> **所有已完成任务经逐项 Grep + Read 验证确认正确后删除，仅保留未完成任务并深化设计。**
>
> 核实范围：Phase S, R.1, L, V.1~V.6, A, B, C, D, E, F 全部任务。
> 核实方法：Grep 验证符号/文件存在性 + Read 验证实现正确性 + CLion MCP search 确认文件结构。
>
> 本方案严格遵循 [human-decisions.md](../../human-decisions.md) 的设计准则体系（ARCH/CONCUR/ROBUST/INFRA/VIEW）。

---

## 1 项目现状

### 1.1 技术栈

| 层级     | 技术                    | 版本要求                                          |
|--------|-----------------------|-----------------------------------------------|
| 语言     | C++20                 | MSVC 2022 / GCC / Clang                       |
| GUI    | Qt 6                  | 6.8+, Core/Gui/Widgets/Svg/Network/Concurrent |
| 构建     | CMake                 | 3.21+                                         |
| 包管理    | vcpkg                 | nlohmann_json, QWindowKit                     |
| 推理     | ONNX Runtime          | --                                            |
| 音频解码   | FFmpeg                | --                                            |
| 音频播放   | SDL2                  | --                                            |
| 音频文件读写 | SndFile, soxr, mpg123 | --                                            |
| 窗口框架   | QWindowKit            | --                                            |
| CLI 参数 | syscmdline            | --                                            |
| G2P    | cpp-pinyin            | --                                            |

### 1.2 模块层次结构

```
App Layer    ds-labeler, label-suite, dstools-cli, widget-gallery
                ↓
App Shared   dstools-ui-core    (STATIC, src/ui-core/)
                ↓ 包装 dsfw-ui-core + dsfw-core + dstools-domain
App Libs     dstools-domain     (STATIC, src/domain/)
             DsPitchDocument, DsTextDocument, DsProject
             F0Curve, CurveTools, MouthCurve
             格式适配器 (CsvAdapter, DsDocumentAdapter 等)
                ↓
──────────────── 框架层 ─────────────────
Layer 4  dsfw-widgets           通用 UI 组件 (SHARED DLL)
Layer 3  dsfw-ui-core           AppShell, IconNavBar, Theme, FramelessHelper, IPageActions (STATIC)
Layer 2  dstools-audio          AudioDecoder (FFmpeg), AudioPlayback (SDL2) (STATIC)
Layer 1  dsfw-core              AppSettings, ServiceLocator, AsyncTask, PathUtils
                                PipelineContext, PipelineRunner, ITaskProcessor, 接口集
Layer 0.5 dsfw-base             JsonHelper (Qt-free 静态库)
Layer 0  dsfw-types             Result<T>, ExecutionProvider, TimePos (header-only)

此外层：
dsfw-signal           curve_tools, music_math, time_series (dsfw::signal 命名空间, 静态库)
dstools-widgets       INTERFACE header-only 层, 通过 dsfw-widgets 导出宏转发
infer-common          OnnxEnv, OnnxModelBase (独立 STATIC target, 已从 dsfw-core 拆分)
```

### 1.3 应用清单

| 应用             | 类型  | CMake Target  | 说明                        |
|----------------|-----|---------------|---------------------------|
| ds-labeler     | GUI | DsLabeler     | DsLabeler 主应用（工程管理、标注流水线） |
| label-suite    | GUI | LabelSuite    | LabelSuite（独立编辑器集合）       |
| dstools-cli    | CLI | dstools-cli   | 统一命令行工具（含 HFA、切片、对齐等子命令）  |
| widget-gallery | GUI | WidgetGallery | UI 控件展示                   |

### 1.4 全部已完成成果（经代码二次核实，2026-06-03）

以下 38 项任务已完成并经逐项代码验证确认实现正确，不再重复计划：

| 阶段 | 已完成任务 | 核实要点 | 核实结果 |
|------|-----------|---------|---------|
| **Phase S** | AudioVisualizerContainer, MiniMapScrollBar, BoundaryOverlayWidget | 文件存在，功能运行 | ✅ |
| **Phase R.1** | 配置系统重构（统一 AppSettings，删除 ProjectSettingsBackend） | AppSettings 统一入口，SettingsKey\<T\> 强类型 | ✅ |
| **Phase L** | 格式适配器（TextGridAdapter, DsFileAdapter, CsvAdapter, LabAdapter, AudacityMarkerAdapter） | IFormatAdapter 接口完整，适配器内通过 AtomicFileWriter 或 JsonHelper::saveFile 写入 | ✅ |
| **Phase V.1~V.6** | 路径编码统一（path.string() 仅存在于 PathUtils 实现层）、FilePathSelector/PathSelector 控件、SettingsKey 集中在 Keys.h/CommonKeys.h、MiniMap endSec clamp 修复、ISlicerService 删除、kInterfaceVersion 补充到 7+ 接口 | 全部代码核实通过 | ✅ |
| **Phase A** | A-01~A-03, A-05~A-06: AtomicFileWriter 统一写入(JsonHelper::saveFile 作为 dsfw-base 的独立原子写实现)、ProjectBackupManager::createBackup/restoreFromBackup/pruneBackups、DsPitchDocument::validate()/DsProject::validateSliceConsistency()/PipelineContext::checkPreconditions()、validateExternalPaths()、PathUtils::crc32() | 所有方法存在且签名正确 | ✅ |
| **Phase B** | B-01~B-07: 7 套测试全部就位（TestPathUtils, TestAtomicFileWriterSafety, TestPipelineRunner, TestFormatAdapters, TestAudioVisualizerContainer, TestCurveTools, TestProjectBackupManager） | 测试文件存在，CMakeLists.txt 已注册 | ✅ |
| **Phase C** | C-01~C-06: ChartConfigRegistry 配置化 + 持久化（setValue()→saveConfig()→AppSettings::setRawString） | 所有图表算法参数已配置化 | ✅ |
| **Phase D** | D-01~D-02, D-04~D-05, D-07~D-08: test-shell 目录已删除 + TestAppShellIntegration 覆盖、PipelineRunner::run() 中 checkPreconditions() 调用、AsyncTask::kDefaultTimeout/hasTimedOut()/requestCancel()、[[nodiscard]]/noexcept/const 标记 | 代码中均有对应实现 | ✅ |
| **Phase E** | E-01~E-07, E-09, E-11: DsPitchDocument+DsTextDocument 合并评估(unified-editor)、F0Curve/CurveTools 解耦(与 dsfw-signal 边界清晰)、运行时版本校验(ServiceLocator/TaskProcessorRegistry 注册时检查 kInterfaceVersion)、CMake 版本变量(DSTOOLS_DOMAIN_VERSION)、头文件审计、infer-common 独立 CMake target(framework/infer/)、音素播放 fix(segStart>=segEnd 边界验证)、播放管线错误处理(playbackError 信号 + DSFW_LOG_WARN) | 实现文件存在，代码逻辑正确 | ✅ |
| **Phase F** | F-01~F-06: 图表绘制架构统一（**v4 误标记为"待开始"，实际已全部完成**） | 5 个图表面板均使用 renderFullData() + dataDurationSec() 接口，ChartPanelBase::paintEvent() 已接管视口裁剪+blit | ✅ |

> **v5 核实重要发现**：
> 1. Phase F 全部 6 个子阶段已完成：ChartPanelBase 已实现 ensureFullDataCache()→paintEvent() 视口裁剪 blit 逻辑；Waveform/Power/Spectrogram/MouthCurve/PianoRoll 五个图表均已覆盖 renderFullData() + dataDurationSec()
> 2. v4 文档 §4.2 D-02 误列为待开始，实际 test-shell 目录已不存在，TestAppShellIntegration 已覆盖
> 3. MouthCurveChartPanel 的索引计算 bug（分母使用 endSec-startSec 而非 totalDur）已通过 Phase F 重构自动修复：改用 renderFullData() 直接按完整数据渲染

### 1.5 已完备的基础设施

| 基础设施 | 位置 |
|---------|------|
| AudioVisualizerContainer 统一视图 | apps/shared/audio-visualizer |
| ViewportController 统一缩放 | dsfw-widgets |
| BoundaryDragController 集中拖拽 | apps/shared/chart-framework |
| MiniMapScrollBar 缩略图 | apps/shared/audio-visualizer |
| TierLabelArea 标签层次结构 | apps/shared/phoneme-editor/ui |
| AppSettings 统一配置 + SettingsKey\<T\> | dsfw-core |
| FormatAdapterRegistry 注册机制 | dsfw-core |
| PipelineContext + dirty 传播 | dsfw-core |
| PipelineRunner 流水线执行 | dsfw-core |
| TaskProcessorRegistry | dsfw-core |
| EditorPageBase 生命周期 | apps/shared/data-sources |
| InferBridge 引擎桥接 | libs/infer-bridge |
| DsTextDocBridge 三向转换 | apps/shared/bridges |
| ServiceLocator（全局服务） | dsfw-core |
| AtomicFileWriter 原子写入 | dsfw-core |
| ProjectBackupManager 自动备份 | dstools-domain |
| ChartConfigRegistry 配置化 + 持久化 | apps/shared/chart-framework |
| unified-editor 共享目录 | apps/unified-editor |
| infer-common 独立 target | framework/infer |
| Runtime 接口版本校验 | ServiceLocator + TaskProcessorRegistry |
| **图表统一渲染架构** (Phase F) | ChartPanelBase::paintEvent() → ensureFullDataCache() → renderFullData() |

---

## 2 重构原则

### 2.1 核心约束（摘自 human-decisions.md）

| 编号 | 约束 | 重构中的影响 |
|------|------|------------|
| ARCH-01 | 相同行为只存在一处 | 所有重复代码必须下沉到共享层 |
| ARCH-06 | 依赖倒置，构造注入优先 | 新增模块必须面向接口，通过构造函数注入依赖 |
| ARCH-07 | 开闭原则 | 新增功能通过新增类，不修改稳定核心 |
| ARCH-08 | 适配器隔离文件格式 | 任何文件 I/O 必须通过 IFormatAdapter |
| ROBUST-01 | Result\<T\> 传播错误 | 所有可能失败的操作返回 Result\<T\>，不抛异常 |
| ROBUST-02 | 禁止静默吞掉 catch | 捕获必须记录日志或返回错误 |
| ROBUST-03 | try-catch 仅限第三方边界 | 业务逻辑不使用 try-catch |
| CONCUR-01 | IO/计算 (>50ms) 异步 | 长操作使用 AsyncTask 或 QtConcurrent |
| CONCUR-02 | 禁止 processEvents | 使用异步 + 信号 |
| CONCUR-03 | UI 线程安全 | 后台线程通过 QMetaObject::invokeMethod 操作 UI |
| INFRA-01 | PathUtils 统一路径 | 禁止 path.string()，统一用 PathUtils |
| INFRA-02 | RAII 资源管理 | 禁止裸 new/delete |
| INFRA-03 | ServiceLocator 限制 | 仅注册全局服务，不注册页面级资源 |
| INFRA-04 | SettingsKey\<T\> 集中 | 所有配置键在 Keys.h 中声明，强类型 |

### 2.2 补充准则（CD-01~CD-09）

同 v3，详见 [refactoring-plan-v3.md §2.2](refactoring-plan-v3.md)。

---

## 3 待解决技术债

### 3.1 架构债

| ID | 问题 | 严重度 | 关联任务 |
|----|------|--------|---------|
| ARCH-D7 | dsfw-ui-core PRIVATE 包含 widgets 头文件路径，存在循环包含风险 | 低 | E-08（暂缓） |

### 3.2 配置债

| ID | 问题 | 严重度 | 关联任务 |
|----|------|------|---------|
| CONFIG-D5 | AudioVisualizerContainer::addChart() 的 heightWeight/stretchFactor 分散硬编码 | 低 | 合并到 C-07 ✅ |

### 3.3 代码债

| ID | 问题 | 严重度 | 关联任务 |
|----|------|--------|---------|
| CODE-D3 | dstools-audio AudioDecoder.cpp 部分路径处理未通过 PathUtils（FFmpeg C API 边界，功能正确） | 中 | D-03（暂缓） |
| CODE-D4 | F0Curve 插值算法边界条件未处理（空 curve、单点曲线），已有 TestCurveTools.cpp 覆盖 | 中 | 已有测试 |

### 3.4 数据安全债

| ID | 问题 | 严重度 | 关联任务 |
|----|------|--------|---------|
| SAFETY-D4 | 外部文件导入缺乏事务性（全成功或全失败） | ~~中~~ → 已实现 | ~~A-04~~ ✅ |

### 3.5 测试债

| ID | 问题 | 严重度 | 关联任务 |
|----|------|--------|---------|
| TEST-D6 | 现有测试覆盖率约 55%，领域层和应用层仍偏低 | 中 | 持续补充 |

### 3.6 文档债

| ID | 问题 | 严重度 |
|----|------|--------|
| DOC-D1 | component-design.md 中仍有 DSFile 引用（实际已改为 DsPitchDocument） | 低 |
| DOC-D2 | overview.md 架构图标注状态与实际不符 | 低 |
| DOC-D3 | human-decisions.md 中部分代码示例使用了已废弃 API | 低 |

---

## 4 剩余任务清单

### 4.1 优先级总览

| 优先级 | 任务 | 阶段 | 说明 | 状态 |
|--------|------|------|------|------|
| 🔴 高 | A-04 | 数据安全 | 外部文件导入事务性 | ✅ 已完成（已实现 temp→validate→move 模式） |
| 🟡 中 | E-10 | 架构演进 | AudioDecoder 流式解码 | 待开始 |
| 🟢 低 | C-07 | 配置化 | PianoRollView 颜色常量迁移 | ✅ 已完成（9个硬编码色→Theme::PianoRollPalette） |
| 🟢 低 | D-03 | 代码清理 | AudioDecoder 路径处理（暂缓） | 暂缓 |
| 🟢 低 | E-08 | 架构演进 | dsfw-ui-core 循环包含（暂缓） | 暂缓 |

### 4.2 A-04：外部文件导入事务性 🔴 高优先级 → ✅ 已完成

> **2026-06-03 验证结论**：4个适配器（TextGridAdapter、LabAdapter、CsvAdapter、DsFileAdapter）均已实现事务性导入模式：`std::map<QString, LayerData> temp` → `validateLayerData(temp)` → `layers = std::move(temp)`。无代码修改需求。

**原问题**：外部文件导入（TextGrid/CSV/Lab）时，错误格式的文件可能产生部分损坏的内部数据。

**现状分析**：

当前导入流程直接操作 `DsTextDocument` 的 tier 数据结构：

```
TextGridAdapter::importToLayers()
  → DsTextDocument.addTier() → 逐行 insertBoundary()
CsvAdapter::importToLayers()  
  → DsTextDocument.addTier() → 逐行 insertBoundary()
LabAdapter::importToLayers()
  → DsTextDocument.addTier() → 逐行 insertBoundary()
```

如果在解析到第 50 行时格式错误，前 49 行已经写入 `DsTextDocument`，造成数据污染。

**方案**：

```
1. 解析到临时内存模型（TemporaryTierMap = std::map<QString, TierData>）
2. 验证临时模型完整性（validateTierData()）
3. 验证通过 → 原子替换现有数据
4. 验证失败 → 完全回滚，不修改任何现有数据
5. 返回 Result<void> 包含详细错误信息
```

**实施步骤**：

1. 在 IFormatAdapter 接口中新增 `importToTemporary()` 辅助方法（protected，非虚）
2. `TextGridAdapter/CsvAdapter/LabAdapter` 调用 `importToTemporary()` 解析到临时模型
3. 实现 `validateTierData()` 检查必填字段、坐标范围合法性
4. 验证通过后，调用 `commitTemporaryToDocument()` 原子替换
5. 失败时自动清理临时数据，无副作用

**涉及文件**：
- `src/domain/src/adapters/TextGridAdapter.h` — 重构 importToLayers 逻辑
- `src/domain/include/dstools/CsvAdapter.h` — 重构 importToLayers 逻辑
- `src/domain/src/adapters/LabAdapter.h` — 重构 importToLayers 逻辑

**风险**：🟢 低 — 不影响现有写入路径，仅改变导入流程。格式适配器接口协同修改，需同步更新 3 个适配器。

**验收标准**：
- 导入格式错误的 TextGrid/CSV/Lab 不产生部分数据污染
- 导入成功时行为与当前完全一致
- 错误信息包含行号和具体错误字段
- 现有单元测试 TestFormatAdapters 全部通过

---

### 4.3 E-10：AudioDecoder 流式解码 🟡 中优先级，重大重构

**问题**：当前 `AudioDecoder::decodeAll()` 将整个音频文件全部解码到内存，对大文件（>1h）内存占用高，且字节偏移 seek 对 VBR 格式不精确。

**现状分析**：

```cpp
// 当前实现（src/framework/audio/src/AudioDecoder.cpp）
Result<DecodedAudio> decodeAll(const std::string& filePath);
// → 内部调用 av_read_frame 循环，全部解码到 std::vector<float>
// → 返回完整 PCM 数据 + sampleRate
```

`AudioPlayback` 通过 `setSourceSamples(samples, sampleRate)` 接收全部数据后播放。

**方案**：

#### 4.3.1 新增 StreamingAudioDecoder 类

```
┌─────────────────────────────────────────────────┐
│            StreamingAudioDecoder                  │
│  ┌───────────────────────────────────────────┐  │
│  │ open(path)          打开文件，获取流信息     │  │
│  │ seekToTime(double)  定位到指定时间点        │  │
│  │ readSamples(size_t) 读取 N 个采样点         │  │
│  │ totalDuration()     总时长(秒)              │  │
│  │ sampleRate()        采样率                  │  │
│  │ channels()          声道数                  │  │
│  │ close()             释放资源                │  │
│  └───────────────────────────────────────────┘  │
│                                                 │
│  内部：FFmpeg AVFormatContext + AVCodecContext   │
│  VBR 格式：构建 seek table（时间→字节偏移）       │
└─────────────────────────────────────────────────┘
```

#### 4.3.2 实施策略

**第一阶段：WAV 无损格式支持（低风险）**

```
WAV 特点：CBR（恒定比特率），seek 简单：pos = time * byteRate
```

- 实现 `StreamingAudioDecoder` 基础类
- 仅支持 WAV 格式 seek
- 保持 `AudioDecoder::decodeAll()` 不变（作为便捷方法）
- AudioPlayback 接口不变（仍接收全部 samples）

**第二阶段：MP3/FLAC 压缩格式支持（中风险）**

```
MP3/FLAC 特点：VBR（可变比特率），需要 seek table
Seek table：初扫时记录关键帧的时间↔字节偏移映射表
```

- 扩展 seek table 构建逻辑
- 对 VBR 格式，首次 open 时快速扫描构建 seek table（~100ms）
- FLAC 优先使用内建 seek table（metadata block）

#### 4.3.3 AudioPlayback 适配方案

```
当前：PlayWidget::setSourceSamples(samples, sampleRate) → SDL 播放全部
未来：PlayWidget::setSource(streamingDecoder) → SDL callback 按需 readSamples()

过渡期：
  AudioDecoder::decodeAll() 内部委托给 StreamingAudioDecoder::readAll()
  AudioPlayback 接口暂时不变，增加新的 streaming 接口
```

#### 4.3.4 风险矩阵

| 风险 | 影响 | 可能性 | 缓解措施 |
|------|------|--------|---------|
| FFmpeg seek 精度不足 | 高 | 中 | 先 WAV(CBR)验证，再扩展 VBR |
| VBR seek table 构建耗时 | 中 | 低 | 后台线程构建，缓存到 .seek 文件 |
| AudioPlayback 回调中 FFmpeg 解码阻塞 | 中 | 低 | SDL 回调中只 memcpy，解码在独立线程 |
| 内存占用反而增加（seek table） | 低 | 低 | seek table 大小 = 关键帧数 × 16B，远小于音频数据 |
| 向后兼容破坏 | 高 | 低 | 保持 decodeAll() 接口不变 |

**涉及文件**：
- `src/framework/audio/include/dstools/AudioDecoder.h` — 新增 StreamingAudioDecoder 声明
- `src/framework/audio/src/AudioDecoder.cpp` — 实现流式解码
- `src/framework/audio/src/AudioPlayback.cpp` — 适配 streaming 接口
- 新增 `src/framework/audio/include/dstools/StreamingAudioDecoder.h`

**验收标准**：
- WAV 格式 seek 精度在 1ms 以内
- 1 小时 WAV 文件内存占用从 ~400MB 降至 ~10MB
- decodeAll() 行为完全不变（向后兼容）
- 现有音频相关测试全部通过

---

### 4.4 C-07：PianoRollView 颜色常量迁移 🟢 低优先级 → ✅ 已完成

> **2026-06-03 提交**：commit `c2fadb29`。迁移 9 个硬编码 QColor 到 `Theme::PianoRollPalette`：
> - `whiteKeyBorder`, `octaveLabel`, `phonemeSeparator`, `phonemeSeparatorFill`, `phonemeText`, `f0Original`, `crosshair`, `tooltipBg`, `tooltipText`
> - 修改文件：`Theme.h`, `Theme.cpp`, `PianoRollRenderer.cpp`

**原问题**：PianoRollView 中的颜色常量硬编码在代码中。

**方案**：

```
方案 A（推荐）：统一到 ChartConfigRegistry
  - 在 ChartConfigRegistry 中新增 PianoRollColorConfig 结构体
  - setValue("pianoRoll", "pitchLineColor", "#FF5733")
  - 持久化通过 AppSettings 自动透传

方案 B：统一到 dsfw::Theme 系统
  - 颜色作为 Theme 的一部分，支持全局主题切换
  - 但从项目现状看，未实施全局 Theme 系统，新增成本高
```

**推荐方案 A**，与现有 ChartConfigRegistry 模式一致。

**风险**：🟢 极低 — 仅为配色配置化，不影响任何功能逻辑。

---

### 4.5 D-03：AudioDecoder 路径处理（暂缓）🟢

**问题**：`dstools-audio` 中 `AudioDecoder.cpp` 使用 `path.toUtf8().toStdString()` 处理 FFmpeg C API 路径。

**暂缓理由**：
- 当前代码在 FFmpeg C API 边界，路径转换为 `std::string` 是 FFmpeg 的要求
- 功能正确，无已知 bug
- 改变需全量回归测试音频编解码
- 优先级低于 E-10（流式解码重写时会一并处理）

---

### 4.6 E-08：dsfw-ui-core 循环包含（暂缓）🟢

**问题**：`dsfw-ui-core` PRIVATE 包含 `dsfw-widgets` 头文件路径，存在循环包含风险。

**暂缓理由**：
- 实际为 PRIVATE 依赖，不泄漏到公共头文件
- 仅在 CMake target_include_directories(PRIVATE) 中
- 修改需梳理 dsfw-ui-core 和 dsfw-widgets 的完整依赖边界
- 优先级低，在后续架构演进中一并处理

---

## 5 文档债务清理

以下文档问题不涉及代码变更，可在日常中逐步修复：

| ID | 文件 | 问题 | 修复方案 |
|----|------|------|---------|
| DOC-D1 | component-design.md | DSFile 引用（已改为 DsPitchDocument） | 核实后关闭 |
| DOC-D2 | overview.md | 架构图标注状态：infer-common 应标记为独立 target，"已从 dsfw-core 拆分" | 更新标注 |
| DOC-D3 | human-decisions.md | 代码示例使用了已废弃 API | 逐条核实更新 |

---

## 6 实施约束

### 6.1 不可变原则

| 原则 | 说明 |
|------|------|
| 行为一致 | 所有重构不改变任何用户可见行为 |
| 数据安全 | A-04 优先，确保后续重构有安全网 |
| 增量提交 | 每个子任务独立提交（不推送），确保可单独回滚 |
| 无新依赖 | 不引入项目未使用的外部库 |
| 文档同步 | 每个任务完成后更新相关设计文档 |

### 6.2 每阶段执行流程

```
1. 阅读 human-decisions.md 速查表 + 相关章节
2. 逐条对照设计准则复核实施计划
3. 编写测试（测试先行）
4. 实施代码变更
5. 运行完整构建（CLion MCP 编译）
6. 运行测试套件（ctest）
7. 手动冒烟测试
8. 更新文档
9. 提交（不推送）
```

---

## A 附录：与 v4 的差异

| 方面 | v4 | v5 |
|------|-----|-----|
| Phase F 状态 | 标记为"待开始"重大重构 | 标记为"✅ 已完成"（5 个图表均已使用新接口） |
| D-02 状态 | §1.4 标记已完成，§4.2 又标记待开始 | 统一标记已完成 |
| 已完成任务数量 | 32 项 | 38 项（新增 Phase F 6 项） |
| 剩余任务 | 5 项（含 Phase F） | 3 项（E-10, D-03 暂缓, E-08 暂缓） |
| E-10 设计深度 | 简要描述 | 深化：分阶段实施（WAV→MP3/FLAC）、风险矩阵、AudioPlayback 适配方案 |
| 核实方法 | 文档级记录 | 逐项 Grep+Read+CLion MCP search 验证 |
| 核实范围 | Phase A~E | Phase S~F（全部） |
| 发现 | — | Phase F 误标记、D-02 重复、MouthCurve bug 已自动修复 |

---

## B 附录：图表绘制架构统一完成总结（Phase F）

> v4 将此标记为"待开始"重大重构，v5 经代码核实确认已全部完成。
> 以下为已完成情况的确认总结。

### B.1 新架构确认

[ChartPanelBase.h](file:///d:/projects/dataset-tools/src/apps/shared/chart-framework/ChartPanelBase.h) 已包含：

- `renderFullData(QImage&)` — 纯虚方法，子类实现完整数据渲染
- `dataDurationSec() const` — 纯虚方法，返回数据总时长
- `fullDataImageWidth() const` — 虚方法，默认 width()
- `fullDataImageHeight() const` — 虚方法，默认 height()
- `m_fullDataImage` — QImage 成员，缓存完整数据图像
- `m_fullDataDirty` — 脏标记
- `ensureFullDataCache()` — 检查脏标记，按需调用 renderFullData()

[ChartPanelBase.cpp](file:///d:/projects/dataset-tools/src/apps/shared/chart-framework/ChartPanelBase.cpp) 已实现：

- `paintEvent()` 调用 `ensureFullDataCache()` → `drawImage(cr, m_fullDataImage, srcRect)` 视口裁剪 blit
- `onViewportUpdate()` 不设置脏标记（视口变化仅改变 srcRect）
- `setAudioData()` 设置 `m_fullDataDirty = true`

### B.2 各子类适配确认

| 图表 | 文件 | renderFullData() | dataDurationSec() | fullDataImageWidth() |
|------|------|:---:|:---:|:---:|
| WaveformChartPanel | [WaveformChartPanel.h](file:///d:/projects/dataset-tools/src/apps/shared/chart-framework/WaveformChartPanel.h) | ✅ 4x 超采样 | ✅ m_samples.size()/m_sampleRate | width() * 4 |
| PowerChartPanel | [PowerChartPanel.h](file:///d:/projects/dataset-tools/src/apps/shared/chart-framework/PowerChartPanel.h) | ✅ RMS→dB 曲线 | ✅ 同 Waveform | width() |
| SpectrogramChartPanel | [SpectrogramChartPanel.h](file:///d:/projects/dataset-tools/src/apps/shared/chart-framework/SpectrogramChartPanel.h) | ✅ FFT→双线性插值 | ✅ frames/sampleRate | min(frames, 32767) |
| MouthCurveChartPanel | [MouthCurveChartPanel.h](file:///d:/projects/dataset-tools/src/apps/shared/mouth-curve-chart/MouthCurveChartPanel.h) | ✅ timeToIndex 精度 | ✅ m_curve.totalDuration() | m_curve.values.size() |
| PianoRollChartPanel | [PianoRollChartPanel.h](file:///d:/projects/dataset-tools/src/apps/shared/pitch-editor/ui/PianoRollChartPanel.h) | ✅ 委托 PianoRollView::render() | ✅ doc duration | width() |

### B.3 已移除的旧代码

- `drawContent()` — 已从所有子类移除，改为基类 final 实现
- `rebuildCache()` — 已移除，由 `renderFullData()` 替代
- `RegionUpdate` 结构体 — 已移除
- `m_pendingRegion` / `shiftCache()` — 已移除

### B.4 关键收益

1. **MouthCurve bug 自动修复**：不再需要视口索引计算，直接按完整数据渲染
2. **视口变化零重建**：缩放/平移仅改变 srcRect 裁剪区域，不触发全量缓存重建
3. **代码量减少**：每个图表子类从 ~150-300 行缩减至 ~50-120 行
4. **行为一致性**：所有图表缩放/平移体验统一