# 重构方案 v3

> 2026-05-07
>
> 前置文档：详见 `refactoring-roadmap-v2.md`、`human-decisions.md`

---

## 需求清单

| # | 需求 | 类型 | 优先级 |
|---|------|------|--------|
| 1 | phoneme 界面删除 tierline 区域 | 功能修改 | P0 |
| 2 | hfa 对齐结果严重错误，参考 main 分支修复 | Bug 修复 | P0 |
| 3 | 部分页面标题过长，无边框不能显示完整标题 | Bug 修复 | P1 |
| 4 | 音高界面切换条目时右侧一致显示"请打开文件"，无标注时也应能播放音频 | Bug 修复 | P1 |
| 5 | 导出页面 CSV 预览浅色主题肉色背景+灰色文字，参考 CLion 样式 | 主题修复 | P1 |
| 6 | slicer 文件列表上方按钮背后多了几个无文字显示的按钮 | Bug 修复 | P1 |
| 7 | 各页面文件列表应统一为一个类，根据页面特点启用部分功能 | 重构 | P1 |
| 8 | phoneme 刻度线显示问题，默认比例尺看不懂；要求 slicer 和 phoneme 底层同一类实例 | Bug 修复 | P1 |
| 9 | 整理需求，设计重构方案（复核设计准则）、更新人工决策 | 文档 | P0 |
| 10 | 完善方案，更新任务到路线图、删除已完成任务 | 文档 | P0 |

---

## 详细分析

### 1. phoneme 界面删除 tierline 区域

**现状**：`AudioVisualizerContainer::AudioVisualizerContainer()` 在 44-47 行固定创建了 `TierLabelArea`：
```cpp
m_tierLabelArea = new TierLabelArea(this);
m_tierLabelArea->setViewportController(m_viewport);
m_tierLabelArea->installEventFilter(this);
mainLayout->addWidget(m_tierLabelArea);
```

然后在 `PhonemeEditor::buildLayout()` (PhonemeEditor.cpp:317-320) 替换为 `PhonemeTextGridTierLabel`：
```cpp
m_tierLabel = new PhonemeTextGridTierLabel(m_container);
m_tierLabel->setViewportController(m_viewport);
m_container->setTierLabelArea(m_tierLabel);
```

同时 `PhonemeEditor` 也有 `TierEditWidget` (PhonemeEditor.cpp:322-323) 作为编辑器区域。

**问题**：当前 phoneme 页面有**两层** tier 显示——`PhonemeTextGridTierLabel` 和 `TierEditWidget`。用户要求删除 tierline 区域。考虑到 `TierEditWidget` (IntervalTierView) 承担了编辑功能，`PhonemeTextGridTierLabel` 只起标签显示和层级选择作用，这与 D-15（标签区域）的设计一致，但用户明确要求删除。

**方案**：为 `AudioVisualizerContainer` 添加 `removeTierLabelArea()` 方法，在 phoneme 页面调用，移除 `PhonemeTextGridTierLabel` 及其标签显示区域。`TierEditWidget` 保留。

**复核设计准则**：
- P-01（行为不分散）：移除方法在 `AudioVisualizerContainer` 中实现，不扩散
- 不影响 slicer——slicer 继续使用自己的 `SliceTierLabel`

**设计决策**：[下文 D-37]

---

### 2. hfa 对齐结果严重错误——参考 main 分支修复

**现状分析**：
- Main 分支 HFA 代码位于 `src/apps/HubertFA/util/Hfa.cpp`，读取 `.lab` 文件获取歌词文本
- 重构分支 `hubert-infer` HFA 位于 `src/infer/hubert-infer/src/Hfa.cpp`，直接接受 `lyricsText` 参数
- 核心算法 (HfaModel → AlignmentDecoder → NonLexicalDecoder → 后处理) **完全一致**
- 数据结构和 WordList/Word/Phone 接口 **完全一致**

**根因**：
`buildFaLayers()` 中，**grapheme 层边界只记录 word.start，不记录 word.end 对应的结束边界**。每个 word 只产生一个 `Boundary{pos: word.start}`，最后一个 `endG.pos = words.back().end` 补在最后：

```cpp
// PhonemeLabelerPage.cpp:65-77
Boundary graphemeB;
graphemeB.pos = secToUs(word.start);  // 只有 start
// ...
r.graphemeLayer.boundaries.push_back(std::move(graphemeB));
// ...
// 最后补一个 END
Boundary endG;
endG.pos = secToUs(words.back().end);
```

**问题**：多个 word 的 grapheme 边界之间存在空洞——用户看到 grapheme 层区间不连续，且 phoneme 层也可能因边界位置计算不准而错位。

对比 main 分支的 `WordList` 后处理（`fill_small_gaps`、`add_SP`）——这些操作只在 `HFA::recognize()` 内部执行，但 `buildFaLayers()` 对结果的解释方式可能导致预期行为不同。

**关键是**：虽然 refactor 分支的 HFA 引擎等价于 main 分支，但 **`buildFaLayers()` 中 grapheme 边界生成逻辑可能不正确**。需要确认：

1. 每个 Word 的 `word.phones[0].start` = `word.start`（通常是相等的）
2. 但 `readFaInput()` 只读 grapheme 层，不依赖 FA 输出的 grapheme

**真正的问题可能在于**：HFA 输出的 `WordList` 中，`add_SP()` 插入的 SP word 和 `add_AP()` 插入的 AP word 的 `phones` 向量为空或被跳过。`buildFaLayers()` 中的 `if (word.phones.empty()) continue;` 可能跳过了重要的结构信息。

**修复方案**：
1. 对比 main 分支的 `AlignmentDecoder::decode()` 和 refactor 分支的实现是否完全一致
2. 对比 main 分支的 `NonLexicalDecoder::decode()` 实现是否完全一致
3. 验证 `fill_small_gaps()` 和 `add_SP()` 的行为
4. 确认 `buildFaLayers()` 对 `WordList` 的遍历顺序和边界构造逻辑是否与 main 分支的 TextGrid 输出等价

**目标**：`数据处理过程、算法和模型推理部分要求完全一致`

---

### 3. 页面标题过长被截断

**现状**：`FramelessHelper.cpp:114-116` TitleBar::resizeEvent：
```cpp
m_titleLabel->setGeometry(0, 0, width(), height());
```

标题覆盖整个 title bar 宽度，但 menu bar 在左侧、窗口按钮在右侧，导致标题文字与二者重叠，加上 `Qt::AlignCenter` 对齐，开头几个字被 menu bar 遮挡。

**方案**：动态计算可用宽度。标题 label 的宽度应限制为 `width() - menuBarWidth - buttonsWidth`，水平居中在此有效区域内。

**设计约束**：
- `TitleBar` 的 layout 布局：menuBar (pos 0) + stretch + buttons
- 标题 label 是浮动层 (`WA_TransparentForMouseEvents`, `lower()`)
- 应计算 menuBar 的 `sizeHint().width()` + buttons 总宽度，居中偏移

---

### 4. 音高页面切换条目时右侧显示"请打开文件" / 无标注应能播放

**现状**：`PitchLabelerPage::onSliceSelectedImpl()` (PitchLabelerPage.cpp:105-147)：
```cpp
if (!source()) {
    m_editor->clear();
    return;
}
// ...
if (!audioPath.isEmpty()) {
    m_editor->loadAudio(audioPath, 0.0);  // ← 只在有 audioPath 时加载
}
```

如果 `validatedAudioPath()` 返回空（文件被删除或路径失效），不加载音频 → 用户无法播放。

**问题**：`loadAudio()` 在 `audioPath.isEmpty()` 时直接返回。但源音频文件存在时应当播放。

**修复方案**：在 PitchEditor 中确保即使 `m_currentFile` 为空（无 pitch/MIDI 数据），`PlayWidget` 仍可以播放音频。将 `m_editor->loadAudio()` 从条件中移出，始终执行。

---

### 5. 导出页面 CSV 预览主题问题

**现状**：`ExportPage::buildPreviewTab()` (ExportPage.cpp:162-176)：
```cpp
m_previewTable->setAlternatingRowColors(true);
```
无自定义 stylesheet。全局 QSS 控制 QTableView 颜色。

**问题**：
- Light 主题 `alternate-background-color: #F0F2F5` 近似肉色
- 缺失数据行 `setBackground(QColor(255, 200, 200))` 浅红（硬编码，不跟随主题）

**方案**：
1. 为 `m_previewTable` 添加专有 stylesheet，使用 `Theme::Palette` 的颜色变量
2. `setObjectName("previewTable")` 在 QSS 中定制
3. 缺失数据行改用 theme-aware 颜色（如 `palette.warning` 半透明叠加）
4. 参考 CLion 风格：深色主题用 `#2B2B2B` 背景 + `#A9B7C6` 文字，浅色主题用 `#FFFFFF` + `#313131`

---

### 6. slicer 文件列表上方无文字按钮

**现状**：`DroppableFileListPanel` (DroppableFileListPanel.cpp:31-35) 创建 5 个按钮：
```cpp
m_btnAddDir = new QPushButton(QStringLiteral("📁"), this);  // emoji
m_btnAdd = new QPushButton(QStringLiteral("+"), this);
m_btnRemove = new QPushButton(QStringLiteral("−"), this);    // U+2212
m_btnDiscard = new QPushButton(QStringLiteral("Discard"), this);
m_btnClear = new QPushButton(QStringLiteral("Clear"), this);
```

**问题**：
- "📁" emoji 在某些 Windows 字体/主题下不渲染或渲染为方块 → `m_btnAddDir` 看似无文字
- "−" (U+2212 MINUS SIGN) 在某些字体下与 "+" 外观差异小或显示为空白
- "Discard"/"Clear" 为英文，在整个中文 UI 中突兀，且在 slicer 上下文中可能超出预期宽度

**修复方案**：
- 为 `AudioFileListPanel` 添加 `setButtonVisible()` 机制或定制按钮样式
- 将 emoji 按钮替换为带图标的按钮（从资源图标加载 SVG）
- 改为 `QToolButton` + icon 方案，不依赖系统字体渲染

---

### 7. 文件列表统一为一个类

**现状**：存在两套文件列表系统：
1. `DroppableFileListPanel` → `AudioFileListPanel`（Slicer 使用，有声频筛选+按钮栏+拖放）
2. `SliceListPanel`（EditorPageBase 子类使用，切片列表+进度+脏标记）

**需求**：统一为同一个类的不同实例，按页面特点启用/禁用部分功能。

**方案**：将 `DroppableFileListPanel` 的功能（按钮栏、拖放、文件筛选）逐步合并入 `SliceListPanel`，或新建一个 `UnifiedFileListPanel` 基类，`AudioFileListPanel` 和 `SliceListPanel` 作为子类。

**但注意**：`AudioFileListPanel` 管理**音频文件**（原始 WAV），而 `SliceListPanel` 管理**切片**（项目中的标注片段）。二者的数据模型不同。统一应是**UI 行为层面的统一**（按钮布局风格一致、进度条一致），而非数据层面的强制一致。

**设计决策**：[下文 D-38]

---

### 8. phoneme 刻度线显示问题

**现状**：
- Slicer 默认 resolution = 3000 spx (`SlicerPage.cpp:160`)
- PhonemeEditor 默认 resolution = 800 spx (`PhonemeEditor.cpp:313`)
- `TimeRulerWidget` 根据 PPS 自动选择刻度级别

**问题**：
- 用户说 "默认比例尺看不懂"——phoneme 的 800 spx 在 44100Hz 下 ≈ 55 PPS，刻度线太密/太疏
- 用户要求 "slicer 和 phoneme 底层是同一个类的不同实例"——这与 D-30 的设计一致（同一套 `AudioVisualizerContainer`），但当前 phoneme 和 slicer 都使用 `AudioVisualizerContainer`，行为应当一致

**区别分析**：
| 特性 | Slicer | Phoneme |
|------|--------|---------|
| 默认 resolution | 3000 | 800 |
| TierLabelArea | SliceTierLabel | PhonemeTextGridTierLabel |
| 刻度线实例 | 同一 TimeRulerWidget | 同一 TimeRulerWidget |
| ViewportController | 共享 | 共享 |
| 比例尺计算 | 同一 `updateScaleIndicator()` | 同一 `updateScaleIndicator()` |

理论上刻度线行为一致。问题可能在：
1. `TimeRulerWidget` 的 `findLevel()` 算法对 phoneme 的 800 spx 选择了不合适的级别
2. `updateScaleIndicator()` 中 `msPerDiv = resolution / sampleRate * 80 * 1000` 的结果显示为看不懂的数字

**修复方案**：
- 调整 `TimeRulerWidget::findLevel()` 的阈值 `kMinMinorStepPx`（当前 60px）
- 或者调整刻度级别表 `kLevels` 以更好地匹配 phoneme 编辑场景
- review `updateScaleIndicator()` 的格式化输出，确保用户能看懂

---

### 9-10. 方案复核 + 路线图更新

与所有现有设计准则 (P-01 ~ P-09) 和决策 (D-01 ~ D-36) 复核，确保不冲突。更新路线图，删除已完成任务。

---

## 设计决策

### D-37：AudioVisualizerContainer 支持移除 TierLabelArea

**决策**：为 `AudioVisualizerContainer` 增加 `removeTierLabelArea()` 方法，允许 phoneme 页面移除 `PhonemeTextGridTierLabel` 层级标签区域。标签区域的高度回收给 `TimeRuler` 和 `TierEditWidget`/`chartSplitter` 使用。

**接口**：
```cpp
void AudioVisualizerContainer::removeTierLabelArea() {
    if (!m_tierLabelArea) return;
    auto *layout = qobject_cast<QVBoxLayout *>(this->layout());
    if (!layout) return;
    layout->removeWidget(m_tierLabelArea);
    m_tierLabelArea->deleteLater();
    m_tierLabelArea = nullptr;
    m_boundaryOverlay->setTierLabelGeometry(0, 0);
    updateOverlayTopOffset();
}
```

**影响**：
- `PhonemeEditor::buildLayout()` 在 `setupLayout` 完成后调用 `m_container->removeTierLabelArea()`
- `updateOverlayTopOffset()` 需要处理 `m_tierLabelArea == nullptr`
- `invalidateBoundaryModel()` 中的 `if (m_tierLabelArea) m_tierLabelArea->onModelDataChanged()` 保持 guard 不变
- Slicer 不受影响

**复核准则**：
- ✅ P-01：新增行为收敛在 `AudioVisualizerContainer`，不扩散
- ✅ D-15：标签区域设计不强制要求可见
- ✅ D-30：同一容器不同配置

### D-38：文件列表面板分层设计

**决策**：保留两个面板的独立职责，但统一按钮布局和视觉风格：

- **`DroppableFileListPanel`**（框架层）：通用文件列表，拖放+筛选+按钮栏+进度
- **`SliceListPanel`**（应用层）：切片列表，数据源绑定+进度+脏状态

两者**不强行合并**——因为数据模型不同（原始文件 vs 项目切片）。但按钮栏风格统一、进度条组件复用。

**具体改造**：
1. `AudioFileListPanel` 按钮从 emoji/ASCII 改为 QIcon（使用资源 SVG 图标）
2. `SliceListPanel` 是否启用按钮栏取决于模式（editor/slicer），由参数控制
3. 引入 `FileListButtonBar` 可复用类，统一按钮样式

**影响文件**：
- `DroppableFileListPanel.{h,cpp}` — 添加 `setButtonBarEnabled(bool)`、`setButtonVisible(id, bool)`
- `AudioFileListPanel.h` — 使用新按钮图标
- `SliceListPanel.{h,cpp}` — 可选启用按钮栏

---

## 受影响文件清单

| # | 文件 | 变更 |
|---|------|------|
| 1 | `src/apps/shared/audio-visualizer/AudioVisualizerContainer.{h,cpp}` | 新增 `removeTierLabelArea()` |
| 1 | `src/apps/shared/phoneme-editor/PhonemeEditor.cpp` | `buildLayout()` 末尾调用 `removeTierLabelArea()` |
| 2 | `src/apps/shared/data-sources/PhonemeLabelerPage.cpp` | `buildFaLayers()` 边界构造修复 |
| 2 | `src/apps/shared/data-sources/PhonemeLabelerPage.h` | 可能新增 FA 结果验证方法 |
| 3 | `src/framework/ui-core/src/FramelessHelper.cpp` | `TitleBar::resizeEvent` 标题区域计算 |
| 4 | `src/apps/shared/data-sources/PitchLabelerPage.cpp` | `onSliceSelectedImpl` 无条件加载音频 |
| 5 | `src/apps/ds-labeler/ExportPage.cpp` | `buildPreviewTab()` stylesheet 主题化；`refreshPreview` 缺失数据行颜色 |
| 6 | `src/framework/widgets/src/DroppableFileListPanel.cpp` | 按钮改为 SVG 图标 |
| 6 | `src/apps/shared/data-sources/AudioFileListPanel.h` | 使用图标按钮 |
| 7 | `src/framework/widgets/include/dsfw/widgets/DroppableFileListPanel.h` | 添加按钮显隐控制 |
| 7 | `src/apps/shared/data-sources/SliceListPanel.{h,cpp}` | 统一按钮风格 |
| 8 | `src/framework/widgets/src/TimeRulerWidget.cpp` | `findLevel` 阈值/级别调整 |
| 8 | `src/apps/shared/audio-visualizer/AudioVisualizerContainer.cpp` | `updateScaleIndicator` 格式化 |
| 9-10 | `docs/refactoring-roadmap-v2.md` | 添加新任务，删除已完成任务 |
| 9-10 | `docs/human-decisions.md` | 添加 D-37、D-38 |

---

## 执行顺序

```
任务 9 (方案复核) → [并行] [1, 2, 3, 4, 5, 6, 7, 8] → 任务 10 (路线图更新)
```

各修复独立：
- 1 (remove tierline) + 8 (scale) 都在 AudioVisualizerContainer 层面，注意冲突
- 2 (HFA) 独立
- 3 (title) 独立
- 4 (pitch audio) 独立
- 5 (export theme) 独立
- 6 (slicer buttons) + 7 (file list) 相关，建议先后执行
