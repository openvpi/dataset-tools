# Dataset Tools 重构方案 v4

> 版本 4.0 | 2026-06-03
>
> v4 基于 v3 全面代码核实后重写：所有已完成任务经逐项代码验证确认正确后删除，仅保留未完成任务并深化设计。
> 核实范围：Phase A~E 全部 38 项任务，核实方法：Grep 验证符号存在性 + Read 验证实现正确性。
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
| dstools-cli    | CLI | dstools-cli   | 统一命令行工具（含 HFA、切片、对齐等子命令）  |
| widget-gallery | GUI | WidgetGallery | UI 控件展示                   |

### 1.4 已完成成果（经代码核实，2026-06-03）

以下 32 项任务已完成并经逐项代码验证确认实现正确，不再重复计划：

| 阶段 | 已完成任务 | 核实要点 |
|------|-----------|---------|
| Phase S | 可视化架构统一（AudioVisualizerContainer, MiniMapScrollBar, BoundaryOverlayWidget） | 文件存在，功能运行 |
| Phase R.1 | 配置系统重构（统一 AppSettings，删除 ProjectSettingsBackend） | AppSettings 统一入口 |
| Phase L | 格式适配器基础设施（TextGridAdapter, DsFileAdapter, CsvAdapter, LabAdapter） | IFormatAdapter 接口完整 |
| Phase V.1~V.6 | 路径编码统一、文件选择控件、配置键集中、MiniMap 修复、ISlicerService 删除、接口版本号 | 全部代码核实通过 |
| Phase A | A-01~A-03, A-05~A-06: AtomicFileWriter 统一、ProjectBackupManager、数据校验、外部路径校验、CRC32 | 实现文件存在，方法签名正确 |
| Phase B | B-01~B-07: 7 套测试全部就位 | 测试文件存在，CMakeLists.txt 已注册 |
| Phase C | C-01~C-06: ChartConfigRegistry 配置化 + 持久化 | setValue()→saveConfig()→AppSettings 已实现 |
| Phase D | D-01~D-02, D-04~D-05, D-07~D-08: test-shell 评估+迁移、PipelineRunner 前置条件、AsyncTask 超时、noexcept/const | 代码中均有对应实现 |
| Phase E | E-01~E-07, E-09, E-11: 合并评估、unified-editor、解耦、版本校验、版本号、头文件审计、infer-common 拆分、音素播放修复、播放管线错误处理 | 实现文件存在，代码逻辑正确 |

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
| ARCH-D2 | test-shell 已删除（D-02），不再存在独立测试 target 问题 | 已解决 | D-02 ✅ |
| ARCH-D7 | dsfw-ui-core PRIVATE 包含 widgets 头文件路径，存在循环包含风险 | 低 | E-08（暂缓） |

### 3.2 配置债

| ID | 问题 | 严重度 | 关联任务 |
|----|------|--------|---------|
| CONFIG-D5 | AudioVisualizerContainer::addChart() 的 heightWeight/stretchFactor 分散硬编码 | 低 | 合并到 C-07 |

### 3.3 代码债

| ID | 问题 | 严重度 | 关联任务 |
|----|------|--------|---------|
| CODE-D3 | dstools-audio AudioDecoder.cpp 部分路径处理未通过 PathUtils | 中 | D-03（暂缓） |
| CODE-D4 | F0Curve 插值算法边界条件未处理（空 curve、单点曲线） | 中 | 已有测试 TestCurveTools.cpp |

### 3.4 数据安全债

| ID | 问题 | 严重度 | 关联任务 |
|----|------|--------|---------|
| SAFETY-D4 | 外部文件导入缺乏事务性（全成功或全失败） | 中 | A-04 |

### 3.5 测试债

| ID | 问题 | 严重度 | 关联任务 |
|----|------|--------|---------|
| TEST-D6 | 现有测试覆盖率约 55%，领域层和应用层仍偏低 | 中 | 持续补充 |

### 3.6 文档债

| ID | 问题 | 严重度 |
|----|------|--------|
| DOC-D1 | component-design.md 中仍有 DSFile 引用 | 低 |
| DOC-D2 | overview.md 架构图标注状态与实际不符 | 低 |
| DOC-D3 | human-decisions.md 中部分代码示例使用了已废弃 API | 低 |

---

## 4 剩余任务清单

### 4.1 待开始任务总览

| 优先级 | 任务 | 阶段 | 说明 |
|--------|------|------|------|
| 🔴 高 | A-04 | 数据安全 | 外部文件导入事务性 |
| 🟡 中 | E-10 | 架构演进 | AudioDecoder 流式解码 |
| 🟢 低 | C-07 | 配置化 | PianoRollView 颜色常量迁移 |
| 🟢 低 | D-03 | 代码清理 | AudioDecoder 路径处理（暂缓） |
| 🟢 低 | E-08 | 架构演进 | dsfw-ui-core 循环包含（暂缓） |

### 4.2 未完成任务详细设计

#### A-04：外部文件导入事务性

**问题**：外部文件导入（TextGrid/CSV/Lab）时，错误格式的文件可能产生部分损坏的内部数据。

**方案**：

```
1. 解析到临时内存模型（TemporaryDocument）
2. 验证临时模型完整性（validate()）
3. 验证通过 → 原子替换现有数据
4. 验证失败 → 完全回滚，不修改任何现有数据
5. 返回 Result<void> 包含详细错误信息
```

**涉及文件**：
- `TextGridAdapter::importToLayers()` — 导入逻辑
- `CsvAdapter::importToLayers()` — 导入逻辑
- `LabAdapter::importToLayers()` — 导入逻辑

**实施要点**：
- 在适配器内部创建临时 `std::map<QString, LayerData>`，解析完成后整体替换
- 不修改现有 `LayerData` 直到验证通过
- 增加 `validateLayerData()` 辅助方法，检查必填字段、坐标范围

**风险**：低 — 不影响现有写入路径，仅改变导入流程。

---

#### D-02：test-shell 迁移为 Qt Test 单元测试 ✅ 已完成

**现状**：test-shell 已精简为仅 AppShell + QMenu 下拉测试，无推理测试逻辑（推理测试已在 D-01 提取）。TestAppShellIntegration 已覆盖 AppShell 程序化测试。

**实施**：
1. 删除 `src/apps/test-shell/` 目录及 CMakeLists.txt 注册
2. 更新 `docs/developer/architecture/overview.md` 移除 test-shell 引用
3. 无需新增测试（TestAppShellIntegration 已覆盖）

---

#### E-10：AudioDecoder 流式解码

**问题**：当前 `AudioDecoder::decodeAll()` 将整个音频文件全部解码到内存，对大文件（>1h）内存占用高，且字节偏移 seek 对 VBR 格式不精确。

**方案**：

```
1. 引入 StreamingAudioDecoder 类，支持按需解码
2. 核心 API：
   - seekToTime(double sec) → 定位到指定时间点
   - readSamples(size_t count) → 读取指定数量采样点
   - totalDuration() → 总时长
   - sampleRate() → 采样率
3. 对 VBR 格式构建 seek table（时间→字节偏移映射表）
4. 保持 decodeAll() 作为便捷方法，内部委托给流式解码器
5. 向后兼容：AudioPlayback 接口不变
```

**涉及文件**：
- `src/framework/audio/src/AudioDecoder.cpp` — 核心修改
- `src/framework/audio/include/dstools/AudioDecoder.h` — 接口扩展
- `src/framework/audio/src/AudioPlayback.cpp` — 适配新接口

**风险**：
- 🔴 **高**：FFmpeg API 使用复杂，seek 精度与格式强相关
- 缓解：先支持 WAV（无压缩）作为第一阶段，再扩展 MP3/FLAC
- 必须保持向后兼容，现有调用方不能 broken

---

#### C-07：PianoRollView 颜色常量迁移

**问题**：PianoRollView 中的颜色常量（如 `Colors::pitchLine`, `Colors::phonemeText` 等）硬编码在代码中。

**方案**：

```
1. 在 ChartConfigRegistry 中新增 PianoRoll 颜色配置项
2. 或统一到 dsfw::Theme 系统
3. 优先级：低，当前颜色方案已满足用户需求
```

---

## 5 重大重构：图表绘制架构统一（Phase F）

> 独立重大重构方案，详见 [附录 B](#b-附录图表绘制架构统一重构方案)。

### 5.1 问题核心

所有图表（Waveform/Power/Spectrogram/MouthCurve/PianoRoll）虽继承同一基类 `ChartPanelBase`，但各自实现了完全不同的绘制逻辑。MouthCurveChartPanel 因索引计算公式错误导致缩放时曲线消失，暴露了架构缺陷。

### 5.2 核心思想

**子类负责渲染"完整数据图像"，基类负责视口裁剪、变换、缓存管理。**

当前架构中 `drawContent(coord)` 参数形同虚设（4 个图表 `Q_UNUSED(coord)`），`rebuildCache()` 不强制，视口变化触发缓存重建导致性能问题。新架构将这些职责统一到基类。

### 5.3 风险矩阵

| 风险 | 影响 | 可能性 | 缓解措施 |
|------|------|--------|---------|
| Waveform 绘制行为回归 | 高 | 中 | 先保留旧代码标记 `[[deprecated]]`，对比验证后删除 |
| 频谱图大文件缓存过大 | 中 | 中 | `fullDataImageWidth()` 上限 32767 (QImage 限制) |
| 钢琴卷帘图委托模式不兼容 | 中 | 低 | Phase F 不改造 PianoRollView 内部，仅适配外部接口 |
| 性能回归（完整图像渲染） | 中 | 低 | 视口变化不再触发重建，实际性能可能提升 |
| 垂直缩放与图像裁剪交互 | 低 | 低 | 以中心为锚点保证对称性 |

### 5.4 实施策略

**分 6 个子阶段，每个子阶段独立可回滚：**

| 阶段 | 内容 | 风险 | 可回滚 |
|------|------|------|--------|
| F-01 | 基类接口扩展（新增纯虚方法，暂不改 paintEvent） | 低 | 是 |
| F-02 | WaveformChartPanel 先行迁移 | 中 | 是（保留旧代码） |
| F-03 | PowerChartPanel + MouthCurveChartPanel 迁移 | 中 | 是 |
| F-04 | SpectrogramChartPanel 迁移 | 中 | 是 |
| F-05 | 基类 paintEvent() 接管绘制，drawContent() final | 高 | 否（需积累信心） |
| F-06 | 清理废弃代码 | 低 | 是 |

---

## 6 实施约束

### 6.1 不可变原则

| 原则 | 说明 |
|------|------|
| 行为一致 | 所有重构不改变任何用户可见行为 |
| 数据安全 | Phase A 优先，确保后续重构有安全网 |
| 增量提交 | 每个子任务独立提交（不推送），确保可单独回滚 |
| 无新依赖 | 不引入项目未使用的外部库 |
| 文档同步 | 每个 Phase 完成后更新相关设计文档 |

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

## A 附录：与 v3 的差异

| 方面 | v3 | v4 |
|------|-----|-----|
| 已完成任务 | 作为待办列出，含详细描述 | 移至 1.4 已完成成果表格，仅保留摘要 |
| 技术债 | 列出所有已解决+未解决 | 仅列出未解决 |
| 任务清单 | 全部任务（含已完成） | 仅未完成任务，含深化设计 |
| Phase F | 作为附录 B | 作为独立重大重构方案，含风险矩阵（5.3）和实施策略（5.4） |
| 代码核实 | 文档级记录 | 逐项 Grep+Read 验证后删除，确保文档不包含虚假信息 |

---

## B 附录：图表绘制架构统一重构方案

### B.1 问题陈述

当前所有图表虽然继承自同一基类 `ChartPanelBase`，但各自实现了完全不同的绘制逻辑。口型曲线图（MouthCurveChartPanel）因错误的索引计算公式导致在缩放/平移时曲线消失，暴露出架构层面的根本缺陷：**每个子类都在重复发明视口映射、缓存管理、坐标变换等基础设施，行为不一致是必然结果**。

违背 ARCH-01（相同行为只存在一处）和 ARCH-04（>60% 重叠应合并到基类）。

### B.2 现状分析：五种图表绘制模式

| 图表 | 类型 | rebuildCache() | drawContent() | 使用 coord？ | 数据源 |
|------|------|---------------|---------------|-------------|--------|
| WaveformChartPanel | 1D 波形 | ✅ 全量+增量 | 遍历缓存绘制 | ❌ Q_UNUSED | m_samples (音频) |
| PowerChartPanel | 1D 功率 | ✅ 全量+增量 | 遍历缓存绘制 | ❌ Q_UNUSED | m_samples (音频) |
| SpectrogramChartPanel | 2D 频谱 | ✅ 全量+增量 | 绘制 m_viewImage | ❌ Q_UNUSED | m_samples → FFT |
| MouthCurveChartPanel | 1D 曲线 | ❌ 空实现 | 实时计算索引，**公式错误** | ❌ Q_UNUSED | m_curve (TimeSeries) |
| PianoRollChartPanel | 2D 钢琴卷帘 | ❌ 委托子控件 | 委托 PianoRollView::render | ✅ 传入 coord | DsPitchDocument |

#### 核心问题

1. **视口映射逻辑重复**：Waveform、Power、Spectrogram 各自独立计算，同一逻辑在 3 个文件中重复
2. **drawContent(coord) 参数形同虚设**：4 个图表 `Q_UNUSED(coord)`，直接读取 `m_converter` 成员变量
3. **rebuildCache() 不强制**：MouthCurve 空实现，PianoRoll 不需要，基类未提供默认实现
4. **两阶段渲染不一致**：Waveform/Power/Spectrogram 采用"缓存→绘制"，MouthCurve 采用"实时计算"
5. **关注点未分离**：视口数学、幅度缩放、数据渲染混杂在子类中

#### 口型曲线 Bug 根因

[MouthCurveChartPanel.cpp:L91-L93](file:///d:/projects/dataset-tools/src/apps/shared/mouth-curve-chart/MouthCurveChartPanel.cpp#L91-L93)：

```cpp
int curveIdx = static_cast<int>(t * m_curve.values.size() /
    (m_converter->endSec() - m_converter->startSec()));  // BUG: 分母应为 dataDuration
```

`curveIdx` 分母使用了**视口时长**而非**数据总时长**。当视口从 0~10s 缩放到 2~4s 时，`curveIdx` 全部越界，曲线完全消失。

### B.3 统一架构设计

#### 核心思想

**子类负责渲染"完整数据图像"，基类负责视口裁剪、变换、缓存管理。**

```
┌─────────────────────────────────────────────────┐
│                 ChartPanelBase                   │
│  ┌───────────────────────────────────────────┐  │
│  │  paintEvent()                             │  │
│  │  1. 检查 m_fullDataDirty → 调用 renderFullData() │
│  │  2. 计算 srcRect = viewport 在完整图上的区域  │  │
│  │  3. 应用垂直缩放 transform                   │  │
│  │  4. painter.drawImage(destRect, image, srcRect) │
│  │  5. 绘制边界叠加层、Y 轴                      │  │
│  └───────────────────────────────────────────┘  │
│                                                 │
│  纯虚接口 (子类实现):                            │
│  - renderFullData(QImage&)    渲染完整数据到图像   │
│  - dataDurationSec() const    数据总时长(秒)     │
│                                                 │
│  可选虚接口 (子类可覆盖):                         │
│  - fullDataImageWidth()  const  默认: width()  │
│  - fullDataImageHeight() const  默认: height() │
└─────────────────────────────────────────────────┘
```

#### 关键设计决策

**决策 1：视口变化不触发缓存重建**

当前架构中 `onViewportUpdate()` 设置 `m_cacheDirty = true`，导致每次缩放/平移都重建缓存。新架构中，完整图像已涵盖所有数据，视口变化仅改变 `srcRect` 的裁剪区域，无需重建。

**决策 2：drawContent() 从虚函数变为基类 final 实现**

所有子类不再需要 `drawContent()` 覆盖。基类统一实现：从 `m_fullDataImage` 中裁剪视口区域，绘制到 `chartContentRect()`。

**决策 3：移除 RegionUpdate 和增量重建**

由于完整图像一次性渲染，不再需要 `RegionUpdate` 结构体、`m_pendingRegion` 成员、`shiftCache()` 模板方法。

**决策 4：PianoRollChartPanel 保持委托模式**

PianoRollChartPanel 内部使用 `PianoRollView` 子控件，通过 `renderFullData()` 委托给 `PianoRollView::render()`，但 PianoRollView 内部不在本次重构范围内。

### B.4 基类接口变更

```cpp
// ChartPanelBase.h — 新接口

class ChartPanelBase : public QWidget, public IChartPanel {
protected:
    // ========== 新增纯虚方法 ==========
    virtual void renderFullData(QImage& image) = 0;
    virtual double dataDurationSec() const = 0;

    // ========== 新增可选虚方法 ==========
    virtual int fullDataImageWidth() const { return width(); }
    virtual int fullDataImageHeight() const { return height(); }

    // ========== 移除的虚方法 ==========
    // rebuildCache(const RegionUpdate&) — 移除，由 renderFullData() 替代
    // drawContent(QPainter&, const ChartCoordinate&) — 移除，基类实现

    // ========== 新增成员 ==========
    QImage m_fullDataImage;
    bool m_fullDataDirty = true;

    // ========== 修改的方法 ==========
    void onViewportUpdate(const ChartCoordinate& conv, int pixelWidth) override {
        m_converter = &conv;
        m_dataPixelWidth = pixelWidth;
        // 不再设置 m_cacheDirty = true
        update();
    }

    void resizeEvent(QResizeEvent* event) override {
        m_dataPixelWidth = width();
        m_fullDataDirty = true;
        QWidget::resizeEvent(event);
    }

    void paintEvent(QPaintEvent*) override {
        ensureFullDataCache();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        auto cr = chartContentRect();
        if (cr.width() <= 0 || cr.height() <= 0) return;

        painter.save();
        painter.setClipRect(cr);

        if (!m_fullDataImage.isNull() && m_converter) {
            double totalDur = dataDurationSec();
            if (totalDur > 0.0) {
                double fracStart = m_converter->startSec() / totalDur;
                double fracEnd = m_converter->endSec() / totalDur;
                int srcX = static_cast<int>(fracStart * m_fullDataImage.width());
                int srcW = std::max(1,
                    static_cast<int>((fracEnd - fracStart) * m_fullDataImage.width()));
                QRect srcRect(srcX, 0, srcW, m_fullDataImage.height());

                if (supportsVerticalZoom() && m_amplitudeScale != 1.0) {
                    painter.save();
                    painter.translate(0, cr.height() / 2.0);
                    painter.scale(1.0, m_amplitudeScale);
                    painter.translate(0, -cr.height() / 2.0);
                    painter.drawImage(cr, m_fullDataImage, srcRect);
                    painter.restore();
                } else {
                    painter.drawImage(cr, m_fullDataImage, srcRect);
                }
            }
        }

        ChartCoordinate coord;
        if (m_converter) {
            coord.viewStart = m_converter->startSec();
            coord.viewEnd = m_converter->endSec();
            coord.pixelWidth = m_dataPixelWidth;
        }
        paintOutOfBoundsOverlay(painter, cr, coord);
        painter.restore();

        QRect axisRect(0, kYAxisPadding, yAxisWidth(), height() - 2 * kYAxisPadding);
        if (axisRect.width() > 0) {
            painter.save();
            painter.setClipRect(axisRect);
            paintYAxisContent(painter, axisRect);
            painter.restore();
        }
    }

    void ensureFullDataCache() {
        if (!m_fullDataDirty) return;
        int imgW = fullDataImageWidth();
        int imgH = fullDataImageHeight();
        if (imgW > 0 && imgH > 0) {
            m_fullDataImage = QImage(imgW, imgH, QImage::Format_ARGB32);
            m_fullDataImage.fill(Qt::transparent);
            renderFullData(m_fullDataImage);
        }
        m_fullDataDirty = false;
    }
};
```

### B.5 各子类适配要点

#### WaveformChartPanel

- `renderFullData()`: 按列计算 min/max，4x 超采样宽度
- `dataDurationSec()`: `m_samples.size() / m_sampleRate`
- 移除：`rebuildCache()`, `drawContent()`, `drawWaveform()`, `rebuildWaveformCache()`, `m_yMax`, `m_yMin`, `m_rms`

#### PowerChartPanel

- `renderFullData()`: 按列计算 RMS → dB，QPainterPath 绘制曲线
- `dataDurationSec()`: 同 Waveform
- 移除：`rebuildCache()`, `drawContent()`, `drawPower()`, `m_powerCache`

#### MouthCurveChartPanel

- `renderFullData()`: 每个采样点一列，`timeToIndex()` 精确查找
- `fullDataImageWidth()`: `m_curve.values.size()`（每个采样点一列）
- **Bug 自动修复**：不再需要索引计算公式，直接使用 `timeToIndex()`
- 移除：`rebuildCache()`（空实现）, `drawContent()`, `paintCurve()`

#### SpectrogramChartPanel

- `renderFullData()`: 懒计算 FFT，双线性插值
- `fullDataImageWidth()`: `min(m_totalFrames, 32767)`（QImage 限制）
- `fullDataImageHeight()`: `m_numFreqBins`
- 移除：`rebuildCache()`, `drawContent()`, `rebuildViewImage()`, `m_viewImage`

#### PianoRollChartPanel

- 保持委托模式：`renderFullData()` → `m_pianoRoll->render()`
- 不改造 PianoRollView 内部实现

### B.6 架构变更对比

| 维度 | 当前架构 | 新架构 |
|------|---------|--------|
| `drawContent()` | 纯虚，子类各自实现 | 基类 final 实现（裁剪+blit） |
| `rebuildCache()` | 虚函数，可选覆盖 | 移除，由 `renderFullData()` 替代 |
| `RegionUpdate` | 用于增量渲染 | 移除 |
| 视口变化 | 触发 `m_cacheDirty = true` → 重建缓存 | 仅改变 `srcRect`，不触发重建 |
| 子类职责 | 管理缓存 + 处理视口 + 绘制 | 仅提供完整数据图像 |
| 基类职责 | 事件分发 + 空壳虚方法 | 缓存管理 + 视口裁剪 + 变换 + 绘制 |
| 代码量（子类） | 每子类 ~150-300 行 | 每子类 ~50-100 行 |

### B.7 验收标准

1. 口型曲线随 Ctrl+滚轮缩放同步更新，不再消失
2. 波形图、功率图、频谱图行为与重构前完全一致
3. Shift+滚轮纵向缩放所有图表表现一致
4. 拖动 MiniMap 平移视口，所有图表同步跟随
5. `cmake --build --preset release` 编译通过
6. 无新增 clang-tidy 警告
7. 手动冒烟测试通过