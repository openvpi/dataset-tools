# Log 系统、UI 侧边栏、本地化与模块合并设计方案

> 2026-05-06
>
> 前置文档：[refactoring-plan.md](refactoring-plan.md)、[framework-architecture.md](framework-architecture.md)

---

## 1. Log 系统完善

### 1.1 现状

- `dsfw::Logger` 单例已实现：多级别（Trace/Debug/Info/Warning/Error/Fatal）、多 sink、线程安全
- 宏 `DSFW_LOG_INFO("cat", "msg")` 等已定义
- 代码中大量使用 `qWarning()`/`qDebug()` 而非 `DSFW_LOG_*`
- 无 UI sink（日志只到控制台）
- 无文件 sink

### 1.2 设计

#### 1.2.1 Logger 扩展

```cpp
// Log.h 新增
class Logger {
public:
    // 新增：获取最近的 N 条日志（供 UI 显示）
    std::vector<LogEntry> recentEntries(int maxCount = 500) const;

    // 新增：信号转发器，将 LogEntry 转为 Qt 信号
    // （Logger 本身不是 QObject，需要一个桥接对象）
    static QObject *notifier();  // 返回 LogNotifier 单例

signals: // LogNotifier 的信号
    void entryAdded(const LogEntry &entry);
};

// 内部 LogNotifier : QObject
class LogNotifier : public QObject {
    Q_OBJECT
signals:
    void entryAdded(const dstools::LogEntry &entry);
};
```

#### 1.2.2 内置 Sink

| Sink | 位置 | 用途 |
|------|------|------|
| `qtMessageSink()` | 已有 | 转发到 qDebug/qWarning |
| `ringBufferSink(size)` | **新增** | 内存环形缓冲，供 UI 查询最近日志 |
| `fileSink(path)` | **新增** | 写入文件，按日期滚动 |

#### 1.2.3 Category 规范

| Category | 用途 | 示例 |
|----------|------|------|
| `audio` | 音频加载/解码/播放 | `DSFW_LOG_INFO("audio", "Loaded 3.2s @ 44100Hz")` |
| `infer` | 推理引擎加载/运行 | `DSFW_LOG_INFO("infer", "FA completed in 1.2s")` |
| `project` | 工程文件加载/保存 | `DSFW_LOG_WARN("project", "Audio file missing: ...")` |
| `ui` | UI 操作/状态变化 | `DSFW_LOG_DEBUG("ui", "Switched to PhonemeLabeler")` |
| `export` | 导出操作 | `DSFW_LOG_INFO("export", "Exported 42 slices")` |
| `edit` | 编辑操作（undo/redo） | `DSFW_LOG_DEBUG("edit", "Boundary moved +0.05s")` |

#### 1.2.4 迁移策略

- `qWarning() << "..."` → `DSFW_LOG_WARN("category", msg.toStdString())`
- `qDebug() << "..."` → `DSFW_LOG_DEBUG("category", msg.toStdString())`
- 不改变第三方库/Qt 内部的日志
- 分批迁移：先 data-sources/ 和 phoneme-editor/，再 framework/

---

## 2. Log UI 侧边栏

### 2.1 设计

在 `AppShell` 层添加一个可折叠的右侧面板，所有页面共享。

```
┌──────────────────────────────────────────────────────┐
│  MenuBar                                       [📋]  │
├────┬─────────────────────────────────┬───────────────┤
│ N  │                                 │ Log Panel     │
│ a  │     Page Content                │ ┌───────────┐ │
│ v  │     (QStackedWidget)            │ │ [Filter▾] │ │
│    │                                 │ ├───────────┤ │
│ B  │                                 │ │ 10:30:01  │ │
│ a  │                                 │ │ [infer]   │ │
│ r  │                                 │ │ FA done   │ │
│    │                                 │ │           │ │
│    │                                 │ │ 10:30:05  │ │
│    │                                 │ │ [audio]   │ │
│    │                                 │ │ Loaded... │ │
│    │                                 │ └───────────┘ │
├────┴─────────────────────────────────┴───────────────┤
│  StatusBar                                           │
└──────────────────────────────────────────────────────┘
```

### 2.2 组件

```cpp
// dsfw-widgets: LogPanelWidget
class LogPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit LogPanelWidget(QWidget *parent = nullptr);

    void appendEntry(const LogEntry &entry);
    void setMinLevel(LogLevel level);       // UI 过滤
    void setCategoryFilter(const QString &category); // 空 = 全部

private:
    QListWidget *m_list;          // 或 QPlainTextEdit
    QComboBox *m_levelFilter;     // All / Info+ / Warning+ / Error
    QComboBox *m_categoryFilter;  // All / audio / infer / ...
    QPushButton *m_clearButton;
    QPushButton *m_exportButton;  // 导出到文件
};
```

### 2.3 AppShell 集成

```cpp
// AppShell 新增
class AppShell : public QMainWindow {
    // ...
private:
    LogPanelWidget *m_logPanel = nullptr;
    QSplitter *m_contentSplitter = nullptr;  // 包裹 stack + logPanel
    QAction *m_toggleLogAction = nullptr;    // 菜单/工具栏 toggle
};

// 构造函数中：
// hLayout: navBar | contentSplitter[ stack | logPanel ]
// logPanel 默认隐藏，通过 toggle 按钮开关
```

### 2.4 自动留痕的操作

以下操作自动写入 log：

| 操作 | Level | Category | 格式 |
|------|-------|----------|------|
| FA 开始/完成/失败 | Info/Error | `infer` | `"FA started for slice: {id}"` |
| 音高提取/MIDI 转写 | Info/Error | `infer` | `"Pitch extraction: {count} frames"` |
| ASR 识别 | Info/Error | `infer` | `"ASR result: {text}"` |
| 音频加载 | Info | `audio` | `"Loaded {path}: {duration}s @ {rate}Hz"` |
| 工程打开/保存 | Info | `project` | `"Project opened: {path}"` |
| 切片保存 | Debug | `edit` | `"Saved slice: {id}"` |
| 导出 | Info | `export` | `"Exported {n} slices to {dir}"` |
| 批量操作跳过 | Warning | `infer` | `"Skipped {n} slices (missing audio)"` |
| 模型加载 | Info/Error | `infer` | `"Model loaded: HuBERT-FA (0.8s)"` |

---

## 3. 本地化 (i18n)

### 3.1 现状

- 已有 `.ts` 翻译文件（中英双语）
- `data-sources/` 下所有 UI 文字用 `QStringLiteral("中文")` 硬编码
- 需改为 `tr("English source")` + `.ts` 翻译

### 3.2 影响范围

| 文件 | QStringLiteral 数 | 优先级 |
|------|-------------------|--------|
| PitchLabelerPage.cpp | 110 | 高 |
| PhonemeLabelerPage.cpp | 63 | 高 |
| MinLabelPage.cpp | 58 | 高 |
| SlicerPage.cpp | 43 | 高 |
| ExportService.cpp | 28 | 中 |
| SliceListPanel.cpp | 13 | 中 |
| FileDataSource.cpp | 14 | 中 |
| EditorPageBase.cpp | 9 | 低 |
| 其他 | < 10 | 低 |

### 3.3 规则

1. **UI 可见文字** → `tr("English text")`，中文写入 `_zh_CN.ts`
2. **Settings key / 内部标识符** → 保持 `QStringLiteral`，不翻译
3. **日志消息** → 英文，不翻译（日志面向开发者）
4. **格式化字符串** → `tr("Exported %1 slices").arg(n)`

### 3.4 迁移步骤

1. 逐文件将 `QStringLiteral("中文")` → `tr("English equivalent")`
2. 运行 `lupdate` 生成/更新 `.ts` 文件
3. 在 `.ts` 文件中添加中文翻译
4. 运行 `lrelease` 生成 `.qm`

---

## 4. 模块合并分析

### 4.1 当前结构

```
EditorPageBase (base class)
├── MinLabelPage      ← 继承
├── PhonemeLabelerPage ← 继承
└── PitchLabelerPage   ← 继承

SlicerPage             ← 独立 QWidget + IPageActions + IPageLifecycle
ExportPage             ← 独立 QWidget + IPageActions + IPageLifecycle
WelcomePage            ← 独立 QWidget + IPageActions
SettingsPage           ← 独立 QWidget + IPageActions + IPageLifecycle
```

### 4.2 合并机会

| 重复模式 | 涉及页面 | 建议 |
|----------|---------|------|
| `createMenuBar()` 手动构建中文菜单 | 全部 6 页 | 不合并（菜单内容不同），但提取 `addStandardFileMenu()` / `addStandardEditMenu()` helper |
| `onActivated()/onDeactivating()` 手写保存/恢复 splitter | Slicer, PhonemeLabeler, PitchLabeler | Slicer 可改继承 EditorPageBase（但 Slicer 不用 SliceListPanel + IEditorDataSource，结构差异大）|
| `ensureXxxEngine()` + `ensureXxxEngineAsync()` 模式 | Phoneme, Pitch, MinLabel | 已有 `EditorPageBase::loadEngineAsync()`，结构合理 |
| 音频加载/waveform 设置 | Slicer, PhonemeEditor | 已通过 `AudioVisualizerContainer` 统一 |
| Export autoComplete 循环 | ExportPage.cpp | 独立，不需合并 |

### 4.3 建议

1. **不合并 SlicerPage 到 EditorPageBase**：Slicer 不用 slice list panel + IEditorDataSource，强行合并会引入不必要的抽象
2. **提取 MenuBarHelper**：`addFileMenu(bar, saveAction, exitSlot)` / `addEditMenu(bar, undoAction, redoAction, shortcutMgr)` 减少手写菜单样板代码
3. **提取 EnginePreloader**：将 `onAutoInfer()` 中的 preload 配置读取 + dirty 层检查统一到 EditorPageBase
4. **LogPanelWidget 作为 AppShell 层组件**：不放入各页面，而是 AppShell 全局共享

### 4.4 不建议合并的项

- `WelcomePage` — 无重复，设计独立
- `SettingsPage` — 无重复
- `ExportPage` — 逻辑完全不同于编辑器页面

---

## 执行顺序

1. **Log 系统** → Logger 扩展 + ringBufferSink + LogNotifier
2. **Log UI** → LogPanelWidget + AppShell 集成
3. **自动留痕** → 在各页面的关键操作点调用 `DSFW_LOG_*`
4. **i18n 迁移** → 逐文件 QStringLiteral → tr()
5. **模块合并** → 视上述分析决定是否执行

---

## 关联文档

- [refactoring-plan.md](refactoring-plan.md) — 重构方案
- [framework-architecture.md](framework-architecture.md) — 项目架构
