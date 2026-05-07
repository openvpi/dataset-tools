# 重构路线图

> 2026-05-07（更新于 2026-05-07）
>
> 前置文档：[human-decisions.md](human-decisions.md)、[unified-app-design.md](unified-app-design.md)
>
> 架构参考：[framework-architecture.md](framework-architecture.md)

---

### 关键架构决策 (ADR)

| ADR | 决策 | 状态 |
|-----|------|------|
| 8 | 单仓库模式 | 有效 |
| 43 | int64 微秒时间精度 | 有效 |
| 46 | LabelSuite + DsLabeler 两个 exe | 有效 |
| 52 | IEditorDataSource 抽象数据源 | 有效 |
| 57 | 层依赖 DAG + per-slice dirty 标记 | 有效 |
| 62 | 右键直接播放，不弹菜单 | 有效 |
| 65 | IBoundaryModel → AudioVisualizerContainer 扩展 | 有效 |
| 66 | LabelSuite 底层统一 dstext | 有效 |
| 69 | ISettingsBackend → 废弃 ProjectSettingsBackend | 有效 |
| 70 | FileDataSource 内部 dstext + FormatAdapter | 有效 |
| 78 | Slicer/Phoneme 统一视图组合（D-30） | 有效 |
| 79 | Log 侧边栏改为独立 LogPage | 有效 |
| 80 | PlayWidget 禁止 ServiceLocator 共享 IAudioPlayer（D-31） | 有效 |
| 81 | invalidateModel 先发信号再卸载（D-32） | 有效 |
| 82 | AudioPlaybackManager 应用级音频仲裁（D-34） | 新 |
| 83 | 贯穿线鼠标光标反馈（D-35） | 新 |
| 84 | 日志持久化 FileLogSink（D-36） | 新 |

---

## 任务：FA 取消机制加固

> 设计依据：P-09（异步任务必须持有引擎存活令牌）
> 审核优化：P-01 要求行为不分散——令牌操作顺序必须严格一致。

### 14.1 FA 异步任务取消令牌加固

**问题**：
- `PhonemeLabelerPage::onDeactivatedImpl()` 中 `m_hfaAlive->store(false)` 后立即 `m_hfaAlive.reset()`
- `m_hfa = nullptr` 在 `reset()` 之后执行，后台线程在 `store(false)` 和 `m_hfa = nullptr` 之间可能访问 `m_hfa`（TOCTOU 间隙）
- P-09 明确指出此方案是"过渡性补救"

**需求**：
- 调整操作顺序：`m_hfaAlive->store(false)` → `m_hfa = nullptr` → `m_hfaAlive.reset()`
- 在 FA lambda 中增加双重检查：先检查 `alive`，再检查 `m_hfa != nullptr`
- 添加 `DSFW_LOG_WARN` 日志记录取消事件

**审核优化（P-01/P-07）**：
- 令牌操作封装为 `EditorPageBase::cancelAsyncTask()` 模板方法，所有推理页面统一调用
- 避免每个页面各写一份 `store(false) + nullptr + reset()` 序列

**文件**：
- 修改：`src/apps/shared/data-sources/EditorPageBase.h`（添加 `cancelAsyncTask` 保护方法）
- 修改：`src/apps/shared/data-sources/EditorPageBase.cpp`
- 修改：`src/apps/shared/data-sources/PhonemeLabelerPage.h`
- 修改：`src/apps/shared/data-sources/PhonemeLabelerPage.cpp`

---

## 任务：贯穿线交互体验增强

> 设计依据：D-05（分割线纵向贯穿规则）、D-35（贯穿线鼠标光标反馈）
> 审核优化：P-01 要求行为不分散——三个 widget 的光标逻辑应统一。

### 11.1 贯穿线区域鼠标光标反馈

**问题**：
- 贯穿线在子图区域（WaveformWidget/SpectrogramWidget/PowerWidget）可拖动
- 但鼠标悬停在贯穿线上时，光标仍为默认箭头，用户无法感知"此处可拖动"
- 仅当鼠标在标签区域（IntervalTierView）内才显示 `SizeHorCursor`

**需求**：
- 在子图 widget 的 `mouseMoveEvent` 中，当 `hitTestBoundary()` 命中边界时，设置 `setCursor(Qt::SizeHorCursor)`
- 当鼠标离开边界区域时，恢复默认光标
- 拖动过程中保持 `SizeHorCursor`
- 三个子图 widget（Waveform/Spectrogram/Power）统一行为

**审核优化（P-01）**：
- 三个 widget 没有共同基类，提取基类是较大重构（违反 P-07 简洁可靠）
- 当前方案：在三个 widget 中统一添加光标逻辑，保持代码结构一致
- 后续路线：待 D-27（BoundaryDragController 统一拖动）实施后，光标逻辑随拖动逻辑一起集中到 controller

**文件**：
- 修改：`src/apps/shared/phoneme-editor/ui/WaveformWidget.cpp`
- 修改：`src/apps/shared/phoneme-editor/ui/SpectrogramWidget.cpp`
- 修改：`src/apps/shared/phoneme-editor/ui/PowerWidget.cpp`

---

## 任务：日志系统持久化

> 设计依据：log-and-i18n-design.md §1.2.2（FileLogSink）、D-36
> 审核优化：P-02（被动接口+容器通知）——FileLogSink 是纯数据 sink，不需要信号。

### 12.1 FileLogSink 日志文件持久化

**问题**：
- `dstools::Logger` 当前仅有 `qtMessageSink` 和 `ringBufferSink`
- 应用崩溃或关闭后，ringBuffer 中的日志全部丢失
- 无法事后排查用户报告的问题

**需求**：
- 新建 `FileLogSink`，将日志写入文件
- 日志文件路径：`dsfw::AppPaths::logsDir()` / `dslabeler_YYYYMMDD.log`
- 自动 7 天轮转：超过 7 天的日志文件自动删除
- 日志格式：`[HH:mm:ss.zzz] [LEVEL] [category] message`
- 在 AppShell 初始化时注册 FileLogSink

**设计约束**：
- FileLogSink 内部使用 `std::mutex` 保护文件写入（Logger 已在内部线程安全）
- 文件 I/O 使用 `std::ofstream`，路径通过 `dstools::toFsPath()` 转换（§12 路径 I/O 规范）
- 不引入新的第三方依赖
- FileLogSink 是纯 sink，不继承 QObject，不发出信号（P-02）

**文件**：
- 新建：`src/framework/core/include/dsfw/FileLogSink.h`、`src/framework/core/src/FileLogSink.cpp`
- 修改：`src/framework/core/include/dsfw/Log.h`（添加 `fileSink()` 工厂方法）
- 修改：`src/framework/core/src/Log.cpp`
- 修改：`src/framework/ui-core/src/AppShell.cpp`（注册 FileLogSink）

---

## 任务：音频系统深度重构

> 设计依据：D-31（每页独立 AudioPlayer）已实施，D-34（AudioPlaybackManager）引入应用级仲裁。
> 审核优化：P-01（行为不分散）+ P-08（禁止 ServiceLocator 误用）。

### 10.1 AudioPlaybackManager 应用级音频仲裁器

**问题**：
- 每个页面各自持有 `PlayWidget`（封装 AudioPlayer），D-31 已确保每页独立实例
- 但页面切换时仅靠 `onDeactivating()` 主动停止播放，无强制仲裁
- 如果 `onDeactivating()` 未被调用（异常路径），音频可能继续播放

**需求**：
- 新建 `AudioPlaybackManager`（QObject，由 AppShell 持有）
- Manager 确保同一时刻只有一个 PlayWidget 在播放——新请求自动停止当前播放者
- Manager 提供 `stopAll()` 方法，供 AppShell 在 `closeEvent` 中调用

**审核优化（P-01/P-08）**：
- PlayWidget 不直接依赖 AudioPlaybackManager——新增 `playRequested()` / `playStopped()` 信号
- AppShell 在页面注册时连接 PlayWidget 信号到 Manager 槽
- Manager 通过调用 `PlayWidget::setPlaying(false)` 停止当前播放者
- 不使用 ServiceLocator，通过 AppShell 构造时建立连接

**文件**：
- 新建：`src/framework/audio/include/dstools/AudioPlaybackManager.h`、`src/framework/audio/src/AudioPlaybackManager.cpp`
- 修改：`src/framework/widgets/include/dsfw/widgets/PlayWidget.h`（新增信号）
- 修改：`src/framework/widgets/src/PlayWidget.cpp`（emit 信号）
- 修改：`src/framework/ui-core/include/dsfw/AppShell.h`（持有 Manager）
- 修改：`src/framework/ui-core/src/AppShell.cpp`（连接信号）

---

## 任务：QSS 主题变量化

> 设计依据：P-06（接口稳定）、P-01（行为不分散）
> 审核优化：P-07（简洁可靠）——占位符方案改动面大，先做最小改动。

### 13.1 QSS 颜色变量提取与统一管理

**问题**：
- `dark.qss` 和 `light.qss` 中硬编码了 30+ 个颜色值
- 修改一个颜色（如 accent）需要在多处查找替换
- 与 `Theme::Palette` 中的颜色定义存在重复（Theme.cpp 和 QSS 各维护一份）

**需求**：
- 在 `Theme::applyTheme()` 中加载 QSS 后，用 `QString::replace()` 替换 `{{placeholder}}` 占位符为 Palette 中的实际颜色值
- QSS 模板中使用 `{{accent}}`、`{{bgPrimary}}` 等占位符
- `Theme::Palette` 中的颜色字段名与占位符名一一对应，Palette 成为唯一真相源

**审核优化（P-07）**：
- 不一次性替换所有硬编码颜色——先提取与 Palette 重复的核心颜色（bgPrimary, bgSecondary, bgSurface, accent, textPrimary, textSecondary, border, borderLight）
- QSS 中仅 Palette 已定义的颜色使用占位符，其余控件特定颜色保留硬编码
- 不引入 SCSS/Less 等预处理工具（保持构建系统简洁）

**文件**：
- 修改：`src/framework/ui-core/res/themes/dark.qss`（核心颜色替换为占位符）
- 修改：`src/framework/ui-core/res/themes/light.qss`（同上）
- 修改：`src/framework/ui-core/src/Theme.cpp`（添加占位符替换逻辑）

---

## 任务依赖关系

```
14.1 (FA取消加固) ── 独立，无依赖
11.1 (贯穿线光标) ── 独立，无依赖
12.1 (FileLogSink) ── 独立，无依赖
10.1 (AudioPlaybackManager) ── 独立，无依赖
13.1 (QSS变量化) ── 独立，无依赖
```

**执行顺序**：14.1 → 11.1 → 12.1 → 10.1 → 13.1
（先修bug再增强，先简单后复杂）

---

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [framework-architecture.md](framework-architecture.md) — 项目架构
- [conventions-and-standards.md](conventions-and-standards.md) — 编码规范
- [log-and-i18n-design.md](log-and-i18n-design.md) — Log 系统与本地化设计
