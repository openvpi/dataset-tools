# 04 — UI 美化与主题系统

**前置**: architecture.md, module-spec.md  
**变更**: v3.0 与架构 v3.0 同步。Pipeline 改为 Tab 分页独立运行，删除全局 Run All 按钮。

本文档定义统一主题系统的设计规范、色板、QSS 规则、自绘组件适配方案、图标体系，以及新应用接入时如何自动获得一致的视觉风格。

---

## 1. 设计目标

1. **暗色优先** — 音频/音乐工具用户普遍偏好暗色界面
2. **一套 QSS 覆盖全部控件** — 新增页面无需额外写样式
3. **自绘组件兼容** — PianoRollView 等 QPainter 自绘组件通过 `Theme::palette()` 获取色板
4. **亮/暗切换** — 用户可选，设置持久化
5. **DPI 适配** — 布局使用相对尺寸，图标使用 SVG

---

## 2. 色板定义

### 2.1 暗色主题 (Dark)

| 变量名 | 色值 | 用途 |
|--------|------|------|
| `bgPrimary` | `#1e1e2e` | 主窗口/页面背景 |
| `bgSecondary` | `#282840` | 面板/列表/输入框背景 |
| `bgSurface` | `#313147` | 卡片/按钮/浮层 |
| `bgHover` | `#3d3d57` | 悬停状态 |
| `accent` | `#7c8af5` | 强调色（按钮/选中/进度条/滑块） |
| `accentHover` | `#8b98ff` | 强调色悬停 |
| `accentPressed` | `#6a78e0` | 强调色按下 |
| `textPrimary` | `#e0e0e8` | 主文字 |
| `textSecondary` | `#8888a0` | 次文字/占位符 |
| `textDisabled` | `#555568` | 禁用文字 |
| `border` | `#3a3a52` | 边框/分隔线 |
| `borderFocus` | `#7c8af5` | 聚焦边框 |
| `success` | `#50c878` | 成功/完成 |
| `error` | `#e05050` | 错误/失败 |
| `warning` | `#f0a050` | 警告/选中高亮 |

### 2.2 亮色主题 (Light)

| 变量名 | 色值 | 用途 |
|--------|------|------|
| `bgPrimary` | `#f5f5f8` | 主背景 |
| `bgSecondary` | `#ffffff` | 面板背景 |
| `bgSurface` | `#e8e8f0` | 按钮/浮层 |
| `bgHover` | `#dcdce8` | 悬停 |
| `accent` | `#5c6ac4` | 强调色 |
| `accentHover` | `#4a58b0` | 强调色悬停 |
| `accentPressed` | `#3d4a9c` | 强调色按下 |
| `textPrimary` | `#1a1a2e` | 主文字 |
| `textSecondary` | `#666680` | 次文字 |
| `textDisabled` | `#aaaabc` | 禁用文字 |
| `border` | `#d0d0dc` | 边框 |
| `borderFocus` | `#5c6ac4` | 聚焦边框 |
| `success` | `#2d8a4e` | 成功 |
| `error` | `#c03030` | 错误 |
| `warning` | `#c07820` | 警告 |

---

## 3. QSS 主题文件

### 3.1 文件结构

```
resources/themes/
├── dark.qss      # 暗色主题完整样式表
└── light.qss     # 亮色主题完整样式表
```

### 3.2 QSS 规范（dark.qss 完整内容）

```css
/* ========== DiffSinger Dataset Tools — Dark Theme ========== */

/* --- Global --- */
QWidget {
    background-color: #1e1e2e;
    color: #e0e0e8;
    font-family: "Segoe UI", "Microsoft YaHei", "Noto Sans CJK SC", sans-serif;
    font-size: 13px;
}

QWidget:disabled {
    color: #555568;
}

/* --- Main Window / Pages --- */
QMainWindow {
    background-color: #1e1e2e;
}

QMainWindow::separator {
    background: #3a3a52;
    width: 1px;
    height: 1px;
}

/* --- Menu Bar --- */
QMenuBar {
    background-color: #1a1a28;
    border-bottom: 1px solid #3a3a52;
    padding: 2px 0;
}

QMenuBar::item {
    padding: 4px 10px;
    border-radius: 4px;
    margin: 2px 2px;
}

QMenuBar::item:selected {
    background-color: #313147;
}

QMenuBar::item:pressed {
    background-color: #7c8af5;
    color: #ffffff;
}

/* --- Menus --- */
QMenu {
    background-color: #282840;
    border: 1px solid #3a3a52;
    border-radius: 8px;
    padding: 6px 4px;
}

QMenu::item {
    padding: 6px 24px 6px 12px;
    border-radius: 4px;
    margin: 1px 4px;
}

QMenu::item:selected {
    background-color: #7c8af5;
    color: #ffffff;
}

QMenu::separator {
    height: 1px;
    background: #3a3a52;
    margin: 4px 8px;
}

QMenu::icon {
    padding-left: 8px;
}

/* --- Push Buttons --- */
QPushButton {
    background-color: #313147;
    border: 1px solid #3a3a52;
    border-radius: 6px;
    padding: 6px 16px;
    color: #e0e0e8;
    min-height: 28px;
    min-width: 60px;
}

QPushButton:hover {
    background-color: #3d3d57;
    border-color: #7c8af5;
}

QPushButton:pressed {
    background-color: #2a2a40;
    border-color: #6a78e0;
}

QPushButton:disabled {
    background-color: #252538;
    border-color: #2e2e42;
    color: #555568;
}

/* Primary action button — 使用 property 选择器 */
QPushButton[cssClass="primary"] {
    background-color: #7c8af5;
    border-color: #6a78e0;
    color: #ffffff;
    font-weight: bold;
}

QPushButton[cssClass="primary"]:hover {
    background-color: #8b98ff;
    border-color: #7c8af5;
}

QPushButton[cssClass="primary"]:pressed {
    background-color: #6a78e0;
}

/* Danger button */
QPushButton[cssClass="danger"] {
    background-color: #5a2020;
    border-color: #802020;
    color: #e05050;
}

QPushButton[cssClass="danger"]:hover {
    background-color: #6a2828;
    border-color: #e05050;
}

/* --- Tool Buttons (icon-only) --- */
QToolButton {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 4px;
}

QToolButton:hover {
    background-color: #313147;
    border-color: #3a3a52;
}

QToolButton:pressed {
    background-color: #7c8af5;
}

/* --- Line Edit / Text Edit --- */
QLineEdit, QTextEdit, QPlainTextEdit {
    background-color: #282840;
    border: 1px solid #3a3a52;
    border-radius: 4px;
    padding: 4px 8px;
    selection-background-color: #7c8af5;
    selection-color: #ffffff;
}

QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
    border-color: #7c8af5;
}

QLineEdit:disabled, QTextEdit:disabled {
    background-color: #222236;
    color: #555568;
}

/* --- Combo Box --- */
QComboBox {
    background-color: #313147;
    border: 1px solid #3a3a52;
    border-radius: 4px;
    padding: 4px 28px 4px 8px;
    min-height: 28px;
}

QComboBox:hover {
    border-color: #7c8af5;
}

QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 24px;
    border-left: 1px solid #3a3a52;
    border-top-right-radius: 4px;
    border-bottom-right-radius: 4px;
}

QComboBox QAbstractItemView {
    background-color: #282840;
    border: 1px solid #3a3a52;
    border-radius: 4px;
    selection-background-color: #7c8af5;
    selection-color: #ffffff;
    outline: none;
}

/* --- Spin Box --- */
QSpinBox, QDoubleSpinBox {
    background-color: #282840;
    border: 1px solid #3a3a52;
    border-radius: 4px;
    padding: 4px 8px;
    min-height: 28px;
}

QSpinBox:focus, QDoubleSpinBox:focus {
    border-color: #7c8af5;
}

/* --- Slider --- */
QSlider::groove:horizontal {
    height: 4px;
    background: #3a3a52;
    border-radius: 2px;
}

QSlider::handle:horizontal {
    width: 14px;
    height: 14px;
    margin: -5px 0;
    background: #7c8af5;
    border-radius: 7px;
}

QSlider::handle:horizontal:hover {
    background: #8b98ff;
    width: 16px;
    height: 16px;
    margin: -6px 0;
    border-radius: 8px;
}

QSlider::sub-page:horizontal {
    background: #7c8af5;
    border-radius: 2px;
}

QSlider::add-page:horizontal {
    background: #3a3a52;
    border-radius: 2px;
}

/* Vertical slider (for PianoRollView scrollbars if needed) */
QSlider::groove:vertical {
    width: 4px;
    background: #3a3a52;
    border-radius: 2px;
}

QSlider::handle:vertical {
    width: 14px;
    height: 14px;
    margin: 0 -5px;
    background: #7c8af5;
    border-radius: 7px;
}

/* --- Progress Bar --- */
QProgressBar {
    background: #282840;
    border: 1px solid #3a3a52;
    border-radius: 6px;
    text-align: center;
    color: #e0e0e8;
    min-height: 20px;
    font-size: 11px;
}

QProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #6a78e0, stop:1 #7c8af5);
    border-radius: 5px;
}

/* --- Tree View / List Widget / Table View --- */
QTreeView, QListWidget, QTableView {
    background-color: #282840;
    alternate-background-color: #2d2d45;
    border: 1px solid #3a3a52;
    border-radius: 4px;
    selection-background-color: #7c8af5;
    selection-color: #ffffff;
    outline: none;
}

QTreeView::item, QListWidget::item {
    padding: 4px 4px;
    border-radius: 2px;
}

QTreeView::item:hover, QListWidget::item:hover {
    background-color: #313147;
}

QTreeView::item:selected, QListWidget::item:selected {
    background-color: #7c8af5;
    color: #ffffff;
}

QTreeView::branch:has-children:!has-siblings:closed,
QTreeView::branch:closed:has-children:has-siblings {
    image: url(:/icons/chevron-right.svg);
}

QTreeView::branch:open:has-children:!has-siblings,
QTreeView::branch:open:has-children:has-siblings {
    image: url(:/icons/chevron-down.svg);
}

/* --- Header View --- */
QHeaderView::section {
    background-color: #282840;
    color: #8888a0;
    border: none;
    border-right: 1px solid #3a3a52;
    border-bottom: 1px solid #3a3a52;
    padding: 6px 8px;
    font-weight: bold;
    font-size: 11px;
    text-transform: uppercase;
}

/* --- Group Box --- */
QGroupBox {
    border: 1px solid #3a3a52;
    border-radius: 8px;
    margin-top: 12px;
    padding: 16px 12px 12px 12px;
    font-weight: bold;
    color: #8888a0;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 12px;
    padding: 0 6px;
    background-color: #1e1e2e;
}

/* --- Tab Widget (Launcher) --- */
QTabWidget::pane {
    border: 1px solid #3a3a52;
    border-radius: 0 0 8px 8px;
    background-color: #1e1e2e;
    top: -1px;
}

QTabBar::tab {
    background-color: #252538;
    border: 1px solid #3a3a52;
    border-bottom: none;
    border-radius: 6px 6px 0 0;
    padding: 8px 20px;
    margin-right: 2px;
    color: #8888a0;
    min-width: 80px;
}

QTabBar::tab:selected {
    background-color: #1e1e2e;
    color: #7c8af5;
    font-weight: bold;
    border-bottom: 2px solid #7c8af5;
}

QTabBar::tab:hover:!selected {
    background-color: #2d2d45;
    color: #e0e0e8;
}

/* --- Scroll Bar --- */
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 0;
}

QScrollBar::handle:vertical {
    background: #4a4a62;
    border-radius: 5px;
    min-height: 30px;
    margin: 2px;
}

QScrollBar::handle:vertical:hover {
    background: #7c8af5;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}

QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: transparent;
}

QScrollBar:horizontal {
    background: transparent;
    height: 10px;
}

QScrollBar::handle:horizontal {
    background: #4a4a62;
    border-radius: 5px;
    min-width: 30px;
    margin: 2px;
}

QScrollBar::handle:horizontal:hover {
    background: #7c8af5;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

/* --- Splitter --- */
QSplitter::handle {
    background: #3a3a52;
}

QSplitter::handle:horizontal {
    width: 2px;
}

QSplitter::handle:vertical {
    height: 2px;
}

QSplitter::handle:hover {
    background: #7c8af5;
}

/* --- Status Bar --- */
QStatusBar {
    background-color: #1a1a28;
    border-top: 1px solid #3a3a52;
    color: #8888a0;
    font-size: 11px;
}

QStatusBar::item {
    border: none;
}

/* --- Tool Tip --- */
QToolTip {
    background-color: #313147;
    border: 1px solid #3a3a52;
    border-radius: 4px;
    color: #e0e0e8;
    padding: 4px 8px;
    font-size: 12px;
}

/* --- Check Box --- */
QCheckBox {
    spacing: 8px;
}

QCheckBox::indicator {
    width: 18px;
    height: 18px;
    border: 2px solid #3a3a52;
    border-radius: 4px;
    background: #282840;
}

QCheckBox::indicator:checked {
    background-color: #7c8af5;
    border-color: #7c8af5;
    image: url(:/icons/check.svg);
}

QCheckBox::indicator:hover {
    border-color: #7c8af5;
}

/* --- Radio Button --- */
QRadioButton::indicator {
    width: 18px;
    height: 18px;
    border: 2px solid #3a3a52;
    border-radius: 9px;
    background: #282840;
}

QRadioButton::indicator:checked {
    background-color: #7c8af5;
    border-color: #7c8af5;
}

/* --- Label (special classes) --- */
QLabel[cssClass="heading"] {
    font-size: 16px;
    font-weight: bold;
    color: #e0e0e8;
}

QLabel[cssClass="caption"] {
    font-size: 11px;
    color: #8888a0;
}

QLabel[cssClass="error"] {
    color: #e05050;
}

QLabel[cssClass="success"] {
    color: #50c878;
}
```

---

## 4. 自绘组件适配

### 4.1 PianoRollView (PitchLabeler) 色板

PianoRollView 使用 QPainter 自绘，不受 QSS 影响。通过 `Theme::palette()` 获取色板：

```cpp
// PianoRollRenderer.cpp
void PianoRollRenderer::render(QPainter &painter, const QRect &rect) {
    const auto &p = dstools::Theme::palette();

    // 背景
    painter.fillRect(rect, p.bgPrimary);

    // 网格线
    QPen gridPen(QColor("#2a2a42"), 1);          // 普通网格
    QPen gridCPen(QColor("#3a3a52"), 1);          // C 音线（加粗）
    QPen gridOctavePen(QColor("#444460"), 1.5);   // 八度线

    // 音符矩形
    QColor noteColor = p.accent;                   // #7c8af5
    QColor noteSlur = QColor("#5c6ad5");           // 连音（稍暗）
    QColor noteRest = QColor("#4a4a62");           // 休止（灰）
    QColor noteSelected = p.warning;               // #f0a050 选中
    int noteRadius = 3;                            // 圆角

    // F0 曲线
    QPen f0Pen(p.error, 2.0, Qt::SolidLine);      // #e05050 红色 2px
    f0Pen.setCosmetic(true);                       // DPI 无关线宽
    painter.setRenderHint(QPainter::Antialiasing);

    // 播放头
    QPen playheadPen(p.success, 1.0);              // #50c878 绿色

    // 音素文本
    QColor phoneColor = p.textSecondary;           // #8888a0
    QFont phoneFont = painter.font();
    phoneFont.setPixelSize(10);

    // 音高标签（钢琴键区域）
    QColor keyLabelColor = p.textSecondary;

    // 十字线
    QColor crosshairColor = QColor(255, 255, 255, 64);  // 半透明白

    // ... 绘制逻辑（保留原 paintEvent 的所有绘制步骤）
}
```

### 4.2 与 PitchLabeler PianoRollView 渲染的对应关系

| 原渲染元素 | 原颜色 | 新颜色 (Dark) | 变化 |
|-----------|--------|---------------|------|
| 背景 | `Qt::white` | `#1e1e2e` | 白→深蓝灰 |
| 网格（细） | `QColor(200,200,200)` | `#2a2a42` | 浅灰→深灰 |
| 网格（C音） | `QColor(160,160,160)` | `#3a3a52` | 中灰→中深灰 |
| 音符填充 | `QColor(80,160,255)` | `#7c8af5` | 蓝→靛蓝紫 |
| 音符边框 | `QColor(40,80,160)` | `#6a78e0` | 深蓝→深靛蓝 |
| F0 曲线 | `Qt::red` | `#e05050` | 纯红→柔红 |
| 音素文本 | `Qt::black` | `#8888a0` | 黑→灰 |
| 音符文本 | `Qt::white` | `#e0e0e8` | 纯白→柔白 |
| 当前音符高亮 | `QColor(255,200,50)` | `#f0a050` | 黄→琥珀 |

---

## 5. 图标体系

### 5.1 图标规范

- **风格**: 线条图标 (Stroke)，1.5px 线宽
- **尺寸**: 24x24 SVG viewBox，实际显示根据 DPI 缩放
- **颜色**: SVG 中使用 `currentColor`，通过 QSS/代码设置颜色
- **来源**: Lucide Icons (MIT license) 或 Tabler Icons (MIT license)

### 5.2 图标清单

| 文件名 | 用途 | 使用位置 |
|--------|------|----------|
| `play.svg` | 播放 | PlayWidget |
| `pause.svg` | 暂停 | PlayWidget |
| `stop-circle.svg` | 停止 | PlayWidget |
| `volume-2.svg` | 音频设备 | PlayWidget |
| `folder-open.svg` | 打开目录 | MinLabel, PitchLabeler |
| `save.svg` | 保存 | 通用 |
| `download.svg` | 导出 | MinLabel |
| `check-circle.svg` | 已完成/已标注 | MinLabel 文件树, TaskWindow |
| `circle.svg` | 未完成/未标注 | MinLabel 文件树 |
| `x-circle.svg` | 失败 | TaskWindow |
| `loader.svg` | 处理中 | TaskWindow |
| `scissors.svg` | 分割音符 | PitchLabeler |
| `merge.svg` | 合并音符 | PitchLabeler |
| `undo.svg` | 撤销 | PitchLabeler |
| `redo.svg` | 重做 | PitchLabeler |
| `plus.svg` | 添加文件 | AudioSlicer, TaskWindow |
| `minus.svg` | 移除 | AudioSlicer, TaskWindow |
| `trash-2.svg` | 清空列表 | AudioSlicer, TaskWindow |
| `settings.svg` | 设置/参数 | 通用 |
| `cpu.svg` | CPU 推理 | GameInfer, HubertFA |
| `gpu.svg` | GPU 推理 | GameInfer, HubertFA |
| `music.svg` | MIDI 输出 | GameInfer |
| `mic.svg` | 音频输入 | 通用 |
| `chevron-right.svg` | 树展开 | QTreeView |
| `chevron-down.svg` | 树收起 | QTreeView |
| `check.svg` | 复选框勾选 | QCheckBox |
| `sun.svg` | 亮色主题 | 设置 |
| `moon.svg` | 暗色主题 | 设置 |

### 5.3 资源文件

```xml
<!-- resources/resources.qrc -->
<RCC>
    <qresource prefix="/themes">
        <file>themes/dark.qss</file>
        <file>themes/light.qss</file>
    </qresource>
    <qresource prefix="/icons">
        <file>icons/play.svg</file>
        <file>icons/pause.svg</file>
        <file>icons/stop-circle.svg</file>
        <!-- ... 全部图标 ... -->
    </qresource>
</RCC>
```

---

## 6. 各应用窗口设计

### 6.1 DatasetPipeline — 三步骤 Tab

```
┌─────────────────────────────────────────────────────────────┐
│ Dataset Pipeline — DiffSinger Dataset Tools                  │
├─────────────────────────────────────────────────────────────┤
│  [Step 1: Slice] │ [Step 2: Lyric Align] │ [Step 3: Phoneme Align] │
│  ─────────────────┴──────────────────────┴──────────────── │
│ ┌─────────────────────────────────────────────────────┐    │
│ │         当前步骤的配置和文件列表区域                    │    │
│ └─────────────────────────────────────────────────────┘    │
│  每个 Tab 页各自有 [▶ Run] / [■ Stop] 按钮             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ Progress: Step 2/3 — LyricFA — 45/128 files  35%    │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ Log output (collapsible)                             │  │
│  └──────────────────────────────────────────────────────┘  │
├──────────────────────────────────────────────────────────┤
│ Ready                                                     │
└──────────────────────────────────────────────────────────┘
```

Tab 样式：
- 选中: 底部 2px `accent` 色边框，文字 `accent` 色
- 未选中: 灰色文字，透明背景
- 悬停: 浅色背景

### 6.2 MinLabel / PitchLabeler / PhonemeLabeler / GameInfer — 独立 QMainWindow

保持各自原有的窗口布局，变化仅限于：
- 深色主题自动应用（通过 `dstools-widgets.dll` 中的 QSS）
- 字体统一（通过 `AppInit::init()`）
- 共享组件（PlayWidget / GpuSelector）替代重复代码
- 各自保留独立的菜单栏、状态栏、窗口大小记忆

---

## 7. 各工具 UI 改进

### 7.1 MinLabel

| 改进 | 实现 |
|------|------|
| 文件树状态图标 | CustomDelegate: 已标注 → `check-circle.svg` (green), 未标注 → `circle.svg` (gray) |
| TextWidget placeholder | `m_contentText->setPlaceholderText(tr("Enter phoneme transcription..."))` |
| 状态栏进度 | `statusBar: "MinLabel — 128/256 (50%) labeled"` |
| G2P 按钮样式 | `cssClass="primary"` 属性 |

### 7.2 PitchLabeler

| 改进 | 实现 | 状态 |
|------|------|------|
| 句子列表颜色标记 | FileListPanel 已实现 3-state 标记（未保存/已修改/已保存） | 已实现 |
| 缩放指示器 | PianoRollView 已有缩放显示 | 已实现 |
| 工具栏按钮 | PitchLabeler 已有 Select/Modulation/Drift/Audition 按钮 | 已实现 |
| 快捷键提示 | 按钮 tooltip 显示快捷键 | 已实现 |

### 7.3 Pipeline Step 1 (AudioSlicer)

| 改进 | 实现 |
|------|------|
| 参数分组 | QGroupBox: "Detection Parameters", "Output Settings" |
| 单位标签 | `threshold` 旁显示 "dB", `minLength` 旁显示 "ms" |
| 参数校验 | 无效值时 border-color → `error`; tooltip 显示约束说明 |
| 文件状态图标 | 待处理(circle), 处理中(loader), 完成(check-circle), 失败(x-circle) |

### 7.4 Pipeline Step 2/3 (LyricFA / HubertFA)

| 改进 | 实现 |
|------|------|
| 日志区域 | QTextEdit 可折叠，默认收起 |
| 进度文件名 | 进度条上方显示当前处理的文件名 |
| 模型加载指示 | 加载时按钮文字变 "Loading..." + 禁用 |

### 7.5 Pipeline 全局

| 改进 | 实现 |
|------|------|
| Tab 状态指示 | Tab 标签旁图标: 就绪(circle), 运行中(loader), 完成(check-circle), 失败(x-circle) |
| 当前 Tab 高亮 | 选中 Tab 底部 2px accent 色边框 |

### 7.6 GameInfer

| 改进 | 实现 |
|------|------|
| 参数表单 | QFormLayout 替代手工布局 |
| 推理进度 | 显示预估剩余时间（基于已处理的 chunk 速率） |

---

## 8. 新应用接入规范

**扩展新的独立工具时，开发者只需：**

### 方式 A: 新建独立 EXE

```cpp
// src/apps/NewTool/main.cpp
#include <QApplication>
#include <dstools/AppInit.h>
#include <dstools/Theme.h>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    dstools::AppInit::init(app);                    // 统一字体
    dstools::Theme::apply(app, dstools::Theme::Dark); // 统一深色主题
    MainWindow w;
    w.show();
    return app.exec();
}
```

```cmake
# src/apps/NewTool/CMakeLists.txt
add_executable(NewTool WIN32 main.cpp MainWindow.cpp)
target_link_libraries(NewTool PRIVATE dstools-widgets Qt6::Core Qt6::Widgets)
```

### 方式 B: 在 Pipeline 中新增步骤

```cpp
// src/apps/pipeline/newtool/NewToolPage.h
class NewToolPage : public dstools::widgets::TaskWindow { ... };
```

在 PipelineWindow 中注册为新 Tab：
```cpp
m_tabWidget->addTab(new NewToolPage(...), tr("Step 4: NewTool"));
```

在 PipelineWindow 中注册为新 Tab，在 Page 内部实现 `runTasks()`。

### 自动获得的样式

无论选择哪种方式，只要链接 `dstools-widgets`：
- QSS 深色/亮色主题自动应用
- `Theme::palette()` 可用于自绘组件
- `PlayWidget`, `TaskWindow`, `GpuSelector` 等共享组件可直接使用
- 图标通过 `QIcon(":/icons/xxx.svg")` 访问
- 字体和 DPI 适配自动生效
- **无需编写任何额外样式代码**

---

## 9. DPI 适配

- QSS 中使用 `px` 值作为基准尺寸（Qt 6 的 High-DPI 缩放会自动处理）
- 图标使用 SVG（矢量缩放）
- 布局使用 `QHBoxLayout/QVBoxLayout/QSplitter`（自动适配）
- 避免固定像素尺寸的 `setFixedSize()`，使用 `setMinimumSize()` + `setSizePolicy()`
- F0Widget 中坐标计算基于 `widget()->width()/height()`，不硬编码像素值（原实现已满足）
