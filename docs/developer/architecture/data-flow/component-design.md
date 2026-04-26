# 核心组件设计说明

> 本文档补充说明项目中关键但之前缺乏设计文档的组件。
> 创建日期：2026-05-22

---

## 1. DSFile — 音高编辑器数据模型

**位置**：[src/apps/shared/pitch-editor/DSFile.h](../../src/apps/shared/pitch-editor/DSFile.h)
**命名空间**：`dstools::pitchlabeler`

### 1.1 设计目标

DSFile 是 PitchEditor 的核心数据载体，存储一个切片内所有音高/音符相关数据。它是一个 **无 Qt GUI 依赖** 的纯内存模型，直接序列化为 JSON。

### 1.2 数据结构

```cpp
struct Phone {           // 音素信息（从 FA 步骤获取）
    QString symbol;      // 音素符号
    TimePos start;       // 相对于切片的起始时间 (µs)
    TimePos duration;    // 持续时间 (µs)
};

struct Note {            // MIDI 音符
    QString name;        // 音高名，如 "C4", "D#5", "rest"
    TimePos duration;    // 持续时长 (µs)
    int slur = 0;        // 是否连音 (1=slur)
    QString glide;       // 滑音类型 ("up", "down", "")
    TimePos start;       // 音符起始时间 (µs)，由 recomputeNoteStarts() 计算
};

class DSFile {
    TimePos offset;                // 偏移量 (µs)
    QString text;                  // 原始歌词文本
    std::vector<Phone> phones;     // 音素列表
    std::vector<Note> notes;       // MIDI 音符列表
    F0Curve f0;                    // F0 时间序列 (int32_t)
    bool modified = false;         // 脏标记
};
```

### 1.3 关键方法

| 方法 | 说明 |
|------|------|
| `recomputeNoteStarts()` | 根据 note[i].duration 重新计算所有 note 的 start 时间 |
| `markModified()` | 设置 modified=true，用于 UI 的保存提示 |
| `getTotalDuration()` | 返回所有 note 的 end() 最大值 |
| `serializeNote(note)` / `deserializeNote(json)` | Note ↔ JSON 序列化 |

### 1.4 与 DsTextDocument 的关系

DSFile 和 DsTextDocument 是**并行关系**，分别服务不同的编辑器：

```
DsTextDocument                       DSFile
├── layers[] (IntervalLayer)         ├── notes[] (Note)
│   ├── grapheme 层                  │   └── name/duration/start/slur/glide
│   ├── phoneme 层                   ├── f0 (F0Curve)
│   ├── ph_num 层                    │   └── values[] (int32_t)
│   └── midi 层（仅 duration）       ├── phones[] (Phone)
├── curves[] (CurveLayer)            │   └── symbol/start/duration
│   └── pitch 层 (int32_t values)    ├── text (歌词)
└── groups/dependencies              ├── offset
                                     └── modified
```

**当前问题**：两者之间没有自动转换桥接，PitchLabelerPage 中手动做 DSFile ↔ DsTextDocument 映射（分散在多处），容易出错。

**建议**：创建 `DsTextDocBridge` 统一管理三向转换（DsTextDocument ↔ DSFile ↔ TextGridDocument）。

---

## 2. DsTextDocument — 统一标注文档格式

**位置**：[src/domain/include/dstools/DsTextTypes.h](../../src/domain/include/dstools/DsTextTypes.h)
**命名空间**：`dstools`
**版本**：3.0.0

### 2.1 设计目标

DsTextDocument 是项目**唯一的数据交换格式**，所有切片数据以 `.dstext` JSON 文件持久化。它是 DsDocument（.ds 文件）的切片级子集。

### 2.2 数据结构

```cpp
struct IntervalLayer {               // 间隔层（文本标注）
    QString name;                    // 层名: "grapheme", "phoneme", "ph_num", "midi"
    QString type;                    // "text" 或 "note"
    std::vector<Boundary> boundaries;

    void sortBoundaries();           // 按 pos 排序
    int nextId() const;              // 获取下一个可用 id
};

struct CurveLayer {                  // 曲线层（数值序列）
    QString name;                    // "pitch"
    TimePos timestep;                // 采样步长 (µs)
    std::vector<int32_t> values;     // 数值序列
};

struct DsTextDocument {
    QString version = "3.0.0";
    DsTextAudio audio;               // 关联音频路径 + in/out 范围
    std::vector<IntervalLayer> layers;
    std::vector<CurveLayer> curves;
    std::vector<BindingGroup> groups;          // 边界绑定组
    std::vector<LayerDependency> dependencies; // 层依赖关系
    DsTextMeta meta;                           // editedSteps 等元数据

    static Result<DsTextDocument> load(const QString &path);
    Result<void> save(const QString &path) const;

    const BindingGroup *findGroup(int boundaryId) const;
    std::vector<int> boundIdsOf(int boundaryId) const;
};
```

### 2.3 层类型约定

| layer.name | type | 含义 | 产生步骤 |
|-----------|------|------|---------|
| `grapheme` | text | 歌词拼音标注 | Step 3 (MinLabel) |
| `phoneme` | text | 音素对齐 | Step 4 (FA) |
| `ph_num` | text | 发音数 | Step 6 (AddPhNum) |
| `midi` | note | MIDI 音符 name+duration | Step 7 (MIDI) |
| `pitch` | curve | F0 时间序列 (int32_t) | Step 8 (Pitch) |

### 2.4 BindingGroup 机制

多个层的边界可以属于同一个 binding group。当用户移动一个边界时，同组的其他层的对应边界也同步移动。`findGroup(boundaryId)` 查找 boundaryId 所在的组。

### 2.5 I/O 安全

- **读取**：`QFile` + `nlohmann::json` 解析，返回 `Result<DsTextDocument>`
- **写入**：`QSaveFile` 原子写入（解决 Windows 文件锁定问题 — BUG-#1）

---

## 3. PianoRollView — MIDI/音高可视化

**位置**：[src/apps/shared/pitch-editor/ui/PianoRollView.h](../../src/apps/shared/pitch-editor/ui/PianoRollView.h)
**命名空间**：`dstools::pitchlabeler::ui`
**继承**：`QFrame`

### 3.1 设计目标

PianoRollView 是 PitchEditor 的**核心渲染组件**，在一个 Canvas 上同时绘制 MIDI 键盘（左侧）、音符块、F0 曲线、波形背景和播放头。

### 3.2 渲染架构

PianoRollView 采用 **被动渲染** 架构（M-VC 分离）：

```
PianoRollView (QFrame)
├── PianoRollInputHandler     ← 鼠标/键盘输入 → 生成 ToolActions
│   ├── SelectTool            → 选择音符、框选
│   ├── ModulationTool        → 滑音调制线编辑
│   ├── OffsetTool            → 音符微调偏移
│   └── AuditionTool          → 试听音符
├── PianoRollRenderer         ← 纯函数渲染器（无状态）
│   ├── drawGrid()            → 小节/拍号网格
│   ├── drawNotes()           → 音符块 (colored rectangles)
│   ├── drawF0Curve()         → F0 曲线叠加
│   ├── drawWaveform()        → 波形背景 (可选)
│   ├── drawPlayhead()        → 播放头竖线
│   ├── drawCrosshair()       → 十字准线
│   ├── drawSnapGuide()       → MIDI 吸附引导线
│   ├── drawRubberBand()      → 框选区域
│   ├── drawRuler()           → 时间刻度 (小节/拍号)
│   └── drawPianoKeys()       → 左侧 MIDI 键盘
└── PitchProcessor            ← F0 处理（重采样/插值）
```

### 3.3 坐标系统

```
Canvas 坐标空间:
    x: 时间轴 (µs)   → 通过 ViewportController 的 CoordinateTransformer 转换
    y: 音高轴 (MIDI) → 通过 midiToY() / yToMidi() 转换

    PianoWidth (px)  = 70px (左侧 MIDI 键盘宽度)
    NoteMinHeight    = 3px  (最小音符高度)
```

### 3.4 ViewportController 集成

```
重构前（P3-3 之前）:
    PianoRollView::timeToX(time) = time * m_hScale + PianoWidth

重构后（P3-3 已完成）:
    PianoRollView 通过 m_viewport (ViewportController*) 委托坐标转换
    setViewportController(VewportController *vc) 替代内部 m_hScale
```

### 3.5 工具模式

PianoRollView 支持 4 种交互模式，通过 [ToolMode 枚举](file:///d:/projects/dataset-tools/src/apps/shared/pitch-editor/ui/PianoRollInputHandler.h) 管理：

| 模式 | 操作 | 快捷键 |
|------|------|--------|
| Select (默认) | 选择/移动/创建/删除音符，拖拽边界 | 双击创建音符 |
| Modulation | 编辑滑音调制曲线 | |
| Offset | 微调音符音高偏移 | |
| Audition | 试听音符音高 | 右键点击音符 |

### 3.6 A/B 对比

PianoRollView 内置 A/B 对比功能（`setABComparisonActive(bool)`），在两个 DSFile 之间切换显示，用于对比编辑前后的 MIDI/F0 差异。

---

## 4. BoundaryDragController — 集中式边界拖拽

**位置**：[src/apps/shared/phoneme-editor/ui/BoundaryDragController.h](../../src/apps/shared/phoneme-editor/ui/BoundaryDragController.h)
**命名空间**：`dstools::phonemelabeler`
**继承**：`QObject`

### 4.1 设计目标

将所有图表组件中重复的边界拖拽逻辑（startDrag/updateDrag/endDrag）提取到单一控制器。由 `AudioVisualizerContainer` 持有，所有 chart widget 通过容器共享。

### 4.2 状态机

```
        mousePress(hitTest=boundary)
                │
                ▼
        startDrag(tier, boundary, model)
           ├── 记录原始位置（source + partners）
           ├── 发现绑定伙伴（跨层同步边界）
           └── emit dragStarted(tier, boundary)
                │
                ▼
        [MouseMove while dragging]
           │
           ▼
        updateDrag(proposedTime)
           ├── clampBoundaryTime(proposed)   → 约束边界范围
           ├── snapToNearestBoundary(time)   → 吸附到临近边界
           ├── moveBoundary(source)          → 预览移动
           └── moveBoundary(partners)        → 绑定伙伴同步
                │
                ▼
        mouseRelease
           │
           ├── endDrag(finalTime, undoStack)
           │      ├── snapToNearestBoundary
           │      ├── restoreOriginals()     → 恢复到起始位置
           │      ├── moveBoundary(source)   → 提交最终位置
           │      ├── moveBoundary(partners) → 提交绑定伙伴
           │      └── undoStack->push(...)   → Undo 命令（若 undoStack 非空）
           │
           └── cancelDrag()
                  └── restoreOriginals()      → 恢复到初始位置
```

### 4.3 绑定(Binding)系统

`findPartners(tier, boundary)` 在 `m_toleranceUs`（默认 1ms）范围内寻找其他层的边界。拖拽 source 时，所有 partners 同步移动，实现**跨层边界绑定**（如 grapheme + phoneme 边界联动）。

### 4.4 Snap 吸附

当 `m_snapEnabled=true`，拖动结束时自动吸附到 `m_snapThresholdUs`（默认 5ms）范围内的最近边界。

### 4.5 Undo 策略

- **容器层**：`AudioVisualizerContainer::eventFilter()` 中 endDrag 传入 `nullptr` undoStack → 容器不做 Undo
- **编辑器层**：各编辑器监听 `dragController::dragFinished` 信号，在信号处理函数中 push Undo 命令
- **原因**：容器不知道页面特有的 Undo 命令类型（MovoeNoteBoundaryCommand vs MoveBoundaryCommand）

### 4.6 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `m_bindingEnabled` | true | 是否启用跨层边界绑定 |
| `m_snapEnabled` | true | 是否启用边界吸附 |
| `m_toleranceUs` | 1000µs | 绑定伙伴发现容忍范围 |
| `m_snapThresholdUs` | 5000µs | 吸附阈值 |
| `m_pixelSnapThreshold` | 10px | 像素级吸附阈值 |

---

## 5. InferBridge — 推理引擎桥接

**位置**：[src/libs/infer-bridge/InferBridge.h](../../src/libs/infer-bridge/InferBridge.h)
**命名空间**：`dstools`
**类型**：header-only template traits

### 5.1 设计目标

封装所有第三方推理引擎类型，提供统一的 `EngineTraits<T>` 模板。apps 层通过 `InferBridge` 间接访问 infer 引擎，**禁止 apps 直接 `#include` infer 库头文件**（ARCH-OP-01）。

### 5.2 桥接的引擎

| 引擎 | 类型 | 用途 | isOpen 检查 |
|------|------|------|------------|
| `HFA::HFA` | Hubert Forced Aligner | 强制对齐 (FA) | `engine->isOpen()` |
| `Rmvpe::Rmvpe` | RMVPE | F0 提取 | `engine->is_open()` |
| `Game::Game` | GAME | MIDI 转录 | `engine->isOpen()` |

### 5.3 使用模式

```cpp
#include <libs/infer-bridge/InferBridge.h>

// 通过 EngineTraits 获取引擎的 ModelProvider 类型
using Provider = dstools::EngineTraits<HFA::HFA>::ProviderType;

// 检查引擎状态
auto engine = provider.model();
if (dstools::EngineTraits<HFA::HFA>::isOpen(engine.get())) {
    engine->align(...);
}
```

### 5.4 与 ITaskProcessor 的关系

InferBridge 是 apps→infer 的静态桥接。Libs 层的具体库（如 hubert-fa/**）封装 ITaskProcessor，内部使用 InferBridge 访问引擎。apps 层通过 `TaskProcessorRegistry` 获取 ITaskProcessor，完全不感知底层引擎。

---

## 关联文档

- [pipeline.md](pipeline.md) — 流水线 I/O 契约
- [ds-format.md](ds-format.md) — PipelineContext JSON 格式 + 层依赖 DAG
- [dirty-mechanism.md](../framework/dirty-mechanism.md) — 标脏传播机制
- [unified-app-design.md](../framework/unified-app-design.md) — 统一应用架构设计
- [overview.md](../overview.md) — 框架架构总览