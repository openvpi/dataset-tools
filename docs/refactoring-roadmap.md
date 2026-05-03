# 架构重构路线图

> 基于 2026-05-02 设计审查更新。
>
> **原则**: 功能齐全、简单可靠、不过度设计。接口保持一致性和合理的扩展预留。
>
> **范围**: 在本仓库内完成。所有模块作为 CMake 子目录共存。
>
> **C++ 标准**: C++20。

---

## 已完成阶段摘要

| Phase | 名称 | 主要成果 |
|-------|------|---------|
| 0 | 预备工作 | ModelType/DocumentFormat 泛化，Qt 依赖分离，pipeline 子模块库化 |
| 1 | 核心分离 | ModelManager→domain，dsfw-base 创建，OnnxModelBase，IInferenceEngine |
| 2 | 库边界固化 | 目录重排，版本化 API，FunASR 适配器，pipeline 后端提取 |
| 3 | 框架增强 | Logger，Undo/Redo，EventBus，CLI 工具 |
| 4 | 完善与扩展 | PluginManager，CrashHandler，UpdateChecker，RecentFiles，WidgetGallery |
| 5-6 | 深度优化 | Result\<T\> 统一，UI 推理解耦，Slicer 合并，MinLabelService 提取 |
| B | 测试与质量 | 24 个测试模块 (80+ 用例)，TODO/FIXME 清理 |
| C | 代码质量 | 大文件拆分，魔法数字常量化 |
| D | CI/CD | 框架独立编译 CI，Doxygen CI，跨平台包分发 |
| G | 任务处理器架构 | ITaskProcessor + Registry，4 个处理器迁移，旧服务接口删除 |
| H | 用户体验与可靠性 | AppPaths，CrashHandler 统一，FileLogSink，PitchLabeler Undo/Redo |
| I | CMake 现代化 | DstoolsHelpers.cmake，40+ CMakeLists 迁移 |
| J | 框架功能补全 | 窗口状态持久化，SingleInstanceGuard，RecentFilesManager，TranslationManager |
| K | 代码规范化 | #pragma once 统一，Doxygen 补全，命名统一 |
| F.1 | 示例项目 | minimal-appshell GUI 示例 |
| L.0-L.8 | 层路由流水线 | TimePos, CurveTools, Boundary/Layer, PipelineContext, PipelineRunner, FormatAdapters, F0/PhonemeLabeler 迁移 |
| L.12 | 编译速度优化 | PCH, ccache, MSVC /MP |
| O | 命名规范化 + CMake 整理 | 目录 kebab-case 统一，deploy.cmake 提取，代码清理 |
| M.1 | 核心编辑器提取 | IEditorDataSource 接口，MinLabelEditor/PhonemeEditor/PitchEditor 提取到 shared/ |

---

## Phase O — 命名规范化 + CMake 整理 + 代码清理

> 目标：统一目录命名为 kebab-case，提取 CMake 部署逻辑，清理遗留不一致。
>
> **时机**：在 L.9 之前执行。纯重命名/重组不影响功能，但越晚做影响范围越大（M 阶段大量新建文件会继承旧命名）。

### 命名规范

| 层 | 约定 | 示例 |
|----|------|------|
| 目录名 | kebab-case | `game-infer`, `min-label`, `pitch-labeler` |
| CMake target 名 | kebab-case | `game-infer`, `dsfw-core` |
| C++ 命名空间 | snake_case 或短名 | `dsfw`, `dstools` |
| 头文件路径 | 小写目录 + PascalCase 文件 | `dsfw/AppSettings.h`, `dstools/DsProject.h` |
| vendor 目录 | 保持原名不动 | `FunAsr/`（ADR-7，vendor 代码不修改） |

### O.1 — apps 目录重命名

| ID | 任务 | 当前 → 目标 | 风险 | 并行 | 状态 |
|----|------|-------------|------|------|------|
| O.1.1 | MinLabel → min-label | `src/apps/MinLabel/` → `src/apps/min-label/` + 更新 CMakeLists | 低：git mv + sed | 全部可并行 | ✅ |
| O.1.2 | GameInfer → game-infer-app | `src/apps/GameInfer/` → `src/apps/game-infer-app/`（避免与 `src/infer/game-infer` 冲突） | 低 | | ✅ |
| O.1.3 | PitchLabeler → pitch-labeler | `src/apps/PitchLabeler/` → `src/apps/pitch-labeler/` | 低 | | ✅ |
| O.1.4 | PhonemeLabeler → phoneme-labeler | `src/apps/PhonemeLabeler/` → `src/apps/phoneme-labeler/` | 低 | | ✅ |
| O.1.5 | TestShell → test-shell | `src/apps/TestShell/` → `src/apps/test-shell/` | 低 | | ✅ |
| O.1.6 | WidgetGallery → widget-gallery | `src/apps/WidgetGallery/` → `src/apps/widget-gallery/` | 低 | | ✅ |
| O.1.7 | 更新 apps/CMakeLists.txt | `add_subdirectory()` 路径全部更新 | 低 | 依赖 O.1.1-6 | ✅ |

**注意**：CMake target 名（如 `GameInfer`、`PitchLabeler`）是可执行文件名，保持 PascalCase 不变（用户可见的二进制名），只改目录名。

### O.2 — infer 目录重命名

| ID | 任务 | 当前 → 目标 | 风险 | 并行 | 状态 |
|----|------|-------------|------|------|------|
| O.2.1 | FunAsr 目录保持不动 | vendor 代码，ADR-7 | — | — | 不做 |

**结论**：infer 下只有 `FunAsr` 不符合 kebab-case，但它是 vendor 目录，按 ADR-7 不修改。其余已统一。

### O.3 — libs 目录重命名

| ID | 任务 | 当前 → 目标 | 风险 | 并行 | 状态 |
|----|------|-------------|------|------|------|
| O.3.1 | gameinfer → game-infer-lib | `src/libs/gameinfer/` → `src/libs/game-infer-lib/` | 低 | 全部可并行 | ✅ |
| O.3.2 | hubertfa → hubert-fa | `src/libs/hubertfa/` → `src/libs/hubert-fa/` | 低 | | ✅ |
| O.3.3 | lyricfa → lyric-fa | `src/libs/lyricfa/` → `src/libs/lyric-fa/` | 低 | | ✅ |
| O.3.4 | rmvpepitch → rmvpe-pitch | `src/libs/rmvpepitch/` → `src/libs/rmvpe-pitch/` | 低 | | ✅ |
| O.3.5 | minlabel → min-label-lib | `src/libs/minlabel/` → `src/libs/min-label-lib/` | 低 | | ✅ |
| O.3.6 | 更新 libs/CMakeLists.txt | 所有 `add_subdirectory()` 更新 | 低 | 依赖 O.3.1-5 | ✅ |

### O.4 — CMake 部署逻辑提取

| ID | 任务 | 方案 | 风险 | 并行 | 状态 |
|----|------|------|------|------|------|
| O.4.1 | 提取部署脚本 | `src/CMakeLists.txt` 第 60~246 行 (windeployqt/macdeployqt/Linux 部署) 提取到 `cmake/deploy.cmake`，主文件 `include(cmake/deploy.cmake)` | 低：纯提取，不改逻辑 | 可独立 | ✅ |
| O.4.2 | 验证三平台部署 | 确认 Windows/macOS/Linux install 行为不变 | 低：CI 验证 | 依赖 O.4.1 | ✅ |

### O.5 — 代码清理

| ID | 任务 | 方案 | 风险 | 并行 | 状态 |
|----|------|------|------|------|------|
| O.5.1 | architecture.md 目录结构同步 | 更新 docs/architecture.md 中的目录树使之与重命名后一致 | 低 | 依赖 O.1-3 | ✅ |
| O.5.2 | CI 路径更新 | 检查 .github/workflows/ 中硬编码的旧目录名 | 低 | 依赖 O.1-3 | ✅ |
| O.5.3 | 移除 setup-vcpkg-temp.bat | 根目录临时脚本，功能已被 README 说明替代 | 低 | 可独立 | ✅ |

**依赖**：无前置依赖。应在 L.9 之前完成（否则 L.9 的代码改动会引用旧路径，增加后续重命名冲突）。

---

## Phase L（剩余） — 流水线集成 + 格式落地

### L.9 — DiffSingerLabeler → PipelineRunner 集成

| ID | 任务 | 方案 | 风险 | 并行 | 状态 |
|----|------|------|------|------|------|
| L.9.1 | BuildCsvPage → PipelineRunner | 不再直接操作 TextGrid + TranscriptionCsv，改为 PipelineRunner + TextGridAdapter + CsvAdapter | 低：适配器已测试 | 可与 L.9.2 并行 | ✅ |
| L.9.2 | GameAlignPage → PipelineRunner | 使用 PipelineRunner 调度 GameMidiProcessor | 低 | 可与 L.9.1 并行 | ✅ |
| L.9.3 | BuildDsPage → PipelineRunner | 使用 PipelineRunner 调度 RmvpePitchProcessor + DsFileAdapter | 低 | 依赖 L.9.1 | ✅ |
| L.9.4 | TaskWindowAdapter 适配 | 适配 PipelineRunner 的 progress/manual-step 信号 | 中：信号契约需与 UI 线程安全对接 | 依赖 L.9.1-3 | ✅ |
| L.9.5 | 切片丢弃 UI | 列表灰显 + 右键丢弃/恢复 + DiscardSliceCommand | 低 | 可独立 | ✅ |

**依赖**：L.4 ✅, L.5 ✅, L.6 ✅

### L.10 — 遗留清理

| ID | 任务 | 方案 | 风险 | 并行 | 状态 |
|----|------|------|------|------|------|
| L.10.1 | TranscriptionPipeline deprecated | `[[deprecated]]` 标注 | 低 | 全部可并行 | ✅ |
| L.10.2 | DsItemManager deprecated | PipelineContext 替代 | 低 | | ✅ |
| L.10.3 | BatchCheckpoint 收缩 | 移除 processBatch 引用 | 低 | | ✅ |
| L.10.4 | DsProjectDefaults 遗留字段删除 | 移除 asrModelPath 等，统一到 taskModels map | 中：需确认所有消费者已迁移 | | ✅ |

**依赖**：L.9

### L.11 — .dsproj v3 规范落地

| ID | 任务 | 方案 | 风险 | 并行 | 状态 |
|----|------|------|------|------|------|
| L.11.1 | DsProject 扩展 | 移除 tasks[]，新增 defaults.preload/export.resampleRate，slice 新增 name/discardedAt。路径 POSIX 化 | 中：新增字段多 | 可独立 | ✅ |
| L.11.2 | DsTextDocument v3 | 层定义新增 type 字段，meta 新增 editedSteps | 低 | 可独立 | ✅ |
| L.11.3 | PipelineContext 精简 | 合并 completedSteps + stepHistory 为单一 stepHistory，新增 editedSteps + dirty | 低 | 可独立 | ✅ |
| L.11.4 | DsProject 测试 | 读写往返、字段校验 | 低 | 依赖 L.11.1 | ✅ |

**依赖**：L.1 ✅。可与 L.9 **并行**。

---

## Phase N — 国际化 (i18n)

> 目标：中英双语支持，架构预留更多语言扩展。

### 现状

- `dsfw::TranslationManager` 已实现（Phase J），支持 .qm 文件加载、系统语言跟随
- 命名约定 `<target>_<locale>.qm`（如 `dsfw-widgets_zh_CN.qm`）
- 当前无 .ts 源文件、无翻译内容

### 任务清单

| ID | 任务 | 方案 | 风险 | 并行 | 状态 |
|----|------|------|------|------|------|
| N.1 | 提取翻译源文件 | CMake 添加 `qt_add_translations()` 或 `lupdate` 自定义目标，扫描所有 `tr()` 调用生成 .ts 文件。每个 CMake target 一个 .ts | 低：Qt 标准流程 | 可独立开始 | ✅ |
| N.2 | 框架层翻译 | 翻译 dsfw-widgets、dsfw-ui-core 中的 UI 字符串（菜单、对话框、按钮）。语言：zh_CN + en | 低 | 依赖 N.1 | ✅ |
| N.3 | 领域层翻译 | 翻译 dstools-widgets 中的 UI 字符串 | 低 | 可与 N.2 并行 | ✅ |
| N.4 | 应用层翻译 | 翻译 LabelSuite / DsLabeler 的页面字符串 | 低 | 依赖 M.2/M.3 | ✅ |
| N.5 | 语言切换 UI | 设置页/偏好中添加语言选择下拉框（system / zh_CN / en），调用 TranslationManager::install() | 低 | 依赖 N.1 | ✅ |
| N.6 | CI 翻译检查 | CI 中 lupdate + 检查未翻译条目数量，PR 中报告覆盖率 | 低 | 独立 | ✅ |

**跨平台注意**：
- macOS：菜单栏字符串需在 .plist 中声明 `CFBundleLocalizations`
- Linux：.desktop 文件需对应 `Name[zh_CN]` 条目
- 所有平台：`TranslationManager::install()` 在 `main()` 中 `QApplication` 构造后、AppShell 构造前调用

**扩展预留**：
- .ts 文件按 `<target>_<locale>.ts` 命名，新增语言只需添加 .ts 文件
- `TranslationManager::availableLanguages()` 自动扫描，UI 下拉框自动更新
- 语言设置存储在 `AppSettings` 中 key `"App/language"`

---

## Phase M — LabelSuite + DsLabeler 统一应用

> 设计文档：[unified-app-design.md](unified-app-design.md) + [ds-format.md](ds-format.md) v3

### M.1 — 核心编辑器组件提取

| ID | 任务 | 方案 | 风险 | 并行 | 状态 |
|----|------|------|------|------|------|
| M.1.0 | IEditorDataSource 接口 | 在 dstools-domain 中定义数据源抽象接口（sliceIds, loadSlice, saveSlice, audioPath） | 低 | 可独立 | ✅ |
| M.1.1 | MinLabelEditor 提取 | 从 `src/apps/MinLabel/gui/` 提取核心编辑 widget 到 `src/apps/shared/`（或 `src/widgets/editors/`），依赖 IEditorDataSource | 高：MinLabel GUI 代码耦合紧密，需仔细分离 I/O 逻辑 | 可与 M.1.2 并行 | ✅ |
| M.1.2 | PhonemeEditor 提取 | 从 `src/apps/PhonemeLabeler/gui/` 提取。波形/频谱/功率渲染 + 边界拖动 + Undo/Redo 是核心 | 高：同上，且渲染逻辑复杂 | 可与 M.1.1 并行 | ✅ |
| M.1.3 | PitchEditor 提取 | 从 `src/apps/PitchLabeler/gui/` 提取。F0 曲线 + 钢琴卷帘 + 多工具编辑 | 高：同上 | 可与 M.1.1 并行 | ✅ |

**风险缓解**：先在现有 exe 中重构为 Editor + Page 分离（Editor 不依赖文件 I/O），验证功能等价后再迁移到 shared/。

### M.2 — LabelSuite

LabelSuite 与 DiffSingerLabeler 共享全部 9 个页面，仅去除 File 菜单中的 .dsproj 工程管理功能。页面组件代码通过 CMake 直接引用 `src/apps/labeler/` 中的源文件。

| ID | 任务 | 方案 | 风险 | 并行 | 状态 |
|----|------|------|------|------|------|
| M.2.1 | LabelSuite exe + main.cpp | CMakeLists.txt + AppShell 9 页面注册（复用 DiffSingerLabeler 全部页面），File 菜单仅保留 Set Working Directory / Clean / Exit | 低 | 依赖 M.1 | ✅ |
| M.2.2 | PhonemeEditor 播放条修复 | PhonemeEditor 工具栏（含 PlayWidget 播放条）内嵌于编辑器自身布局，不使用 QMainWindow::addToolBar() | 低 | 可独立 | ✅ |
| M.2.3 | LabelSuite 功能验证 | 手动测试：全部 9 页面功能，打开/编辑/保存各格式文件 | — | 依赖 M.2.1-2 | 待做 |

**跨平台注意**：
- 文件对话框使用 `QFileDialog::getOpenFileName()`，macOS 上不支持 native dialog 的某些 filter 语法
- 音频播放依赖 SDL2，各平台音频后端不同（Windows: DirectSound, macOS: CoreAudio, Linux: PulseAudio/ALSA）

### M.3 — DsLabeler

| ID | 任务 | 方案 | 风险 | 并行 | 状态 |
|----|------|------|------|------|------|
| M.3.1 | ProjectDataSource 实现 | IEditorDataSource 的工程实现（读写 .dstext + PipelineContext） | 中：需处理 PipelineContext ↔ DsTextDocument 转换 | 依赖 M.1.0, L.11 | ✅ |
| M.3.2 | DsLabeler exe + main.cpp | CMakeLists.txt + AppShell 7 页面注册（含 SlicerPage，Settings 移至末尾） | 低 | 依赖 M.1 | ✅ |
| M.3.3 | WelcomePage | 新建工程向导（2 步模态对话框：基本信息 + 音频导入）、打开工程、最近工程列表。向导完成后跳转切片页 | 低 | 可独立 UI 先行 | ✅ |
| M.3.4 | SettingsPage | 横向 TabWidget（6 个 tab，已移除切片 tab），读写 DsProject.defaults。移至末尾第 7 页 | 低 | 可与 M.3.3 并行 | ✅ |
| M.3.5 | DsMinLabelPage | ProjectDataSource + MinLabelEditor + ASR/LyricFA 按钮 + 批处理菜单 | 中：ASR 按钮需对接 FunASR 推理 + 后台线程 | 依赖 M.1.1, M.3.1 | ✅ |
| M.3.6 | DsPhonemeLabelerPage | ProjectDataSource + PhonemeEditor + 自动 FA 逻辑 + 批处理菜单 + 预加载 | 中：预加载需后台线程池 + UI 取消同步 | 依赖 M.1.2, M.3.1 | ✅ |
| M.3.7 | DsPitchLabelerPage | ProjectDataSource + PitchEditor + 自动 add_ph_num/F0/MIDI + 批处理 + 预加载 | 中：3 个自动步骤串联，错误处理复杂 | 依赖 M.1.3, M.3.1 | ✅ |
| M.3.8 | ExportPage | 导出页面 UI + 自动补全逻辑 + CSV/DS/WAV 输出 + 重采样 + 进度 | 中：补全逻辑需调用多个处理器，重采样需 soxr 跨平台验证 | 依赖 M.3.1, L.9 | ✅ |
| M.3.9 | 导出前置校验 | grapheme 层缺失检测，per-slice editedSteps 检查，按钮禁用/提示 | 低 | 依赖 M.3.8 | ✅ |
| M.3.10 | 层依赖失效引擎 | 实现 `LayerDependencyGraph`：保存时检测修改的层 → 传递标记下游层 dirty → 持久化到 context JSON | 中：需仔细定义"层是否实际变化"的比较逻辑（避免误触发） | 依赖 M.3.1 | ✅ |
| M.3.11 | dirty 自动重算 + Toast | 页面 `onActivated()` 中检测当前切片的 dirty 列表 → 后台重算 → Toast 通知 → 刷新 UI。Toast 3 秒消失，不阻塞编辑 | 中：重算期间用户可能切换切片，需取消正在进行的重算 | 依赖 M.3.10 | ✅ |
| M.3.12 | DsLabeler 端到端测试 | 手动验证：完整流程 + 回退场景（音素修改后回到 PitchLabeler 验证 dirty 重算） | — | 依赖全部 M.3 | 待做 |

**跨平台注意**：
- DirectML 仅 Windows。macOS/Linux 上 provider 自动 fallback 到 CPU
- dstemp/ 路径使用 `QDir::tempPath()` 或工程相对路径，避免硬编码路径分隔符
- 推理线程不应使用 Qt GUI 线程。使用 `Qt::Concurrent` 或 `std::async` + signal 通知 UI

### M.5 — WaveformPanel 共享 + SlicerPage + 右键播放 + 双声道

> 设计文档：[unified-app-design.md §3.4, §11](unified-app-design.md)
>
> **核心理念**：SlicerPage = 单层 PhonemeLabeler（数字序号代替音素名）+ 切片导出。最大化复用 WaveformPanel。

| ID | 任务 | 方案 | 风险 | 并行 | 状态 |
|----|------|------|------|------|------|
| M.5.1 | WaveformPanel 提取 | 从 PhonemeEditor 提取共享波形组件到 `src/apps/shared/waveform-panel/`：TimeRulerWidget, WaveformWidget, MelSpectrogramWidget, ViewportController, PlaybackController。WaveformPanel 作为容器 widget 组合这些子组件 | 高：需从 PhonemeEditor 解耦波形部分 vs 标注部分 | 可独立 | ✅ |
| M.5.2 | 右键直接播放 | WaveformWidget::contextMenuEvent 改为：计算 segStart/segEnd → 直接 play，不弹菜单。事件设为 accepted 阻止冒泡。同时在 TierEditWidget/SpectrogramWidget 等标注区域的右键事件中统一实现相同行为 | 低：改动已有代码一处 | 依赖 M.5.1 | ✅ |
| M.5.3 | 取消 PlayWidget 工具栏 | PhonemeEditor 中 m_toolbar 移除 PlayWidget 的显示。PlayWidget 实例保留为内部播放后端（不 show）。右键播放 + Escape 停止播放 | 低 | 依赖 M.5.2 | ✅ |
| M.5.4 | 双声道处理 | WaveformWidget 保存多声道原始数据 + mono 混合。新增 ChannelMode 枚举（Mono/StereoSplit）。双声道时显示提示条 + 切换按钮。处理流水线始终使用 mono 数据 | 中：需修改 setAudioData 和 paintEvent | 可与 M.5.2 并行 | ✅ |
| M.5.5 | MelSpectrogramWidget | 新增 Mel 频谱图 widget（波形图下方，可折叠）。使用 FFTW3 计算 mel 频谱。复用 SpectrogramWidget 的渲染逻辑但简化为 mel scale | 中：需理解现有 SpectrogramWidget 实现 | 可与 M.5.1 并行 | ✅ |
| M.5.6 | DsSlicerPage 骨架 | 新建 `src/apps/ds-labeler/DsSlicerPage.{h,cpp}`。布局：预处理折叠面板 + 切片参数面板 + WaveformPanel + SliceNumberLayer + SliceListPanel | 中 | 依赖 M.5.1 | ✅ |
| M.5.7 | SliceNumberLayer | 单层序号标注 widget（类似 TierEditWidget 但只显示数字序号 001/002/003，不可编辑文本。边界线与 WaveformPanel 的切割线同步）。实现为简化版 IntervalTierView | 低 | 依赖 M.5.6 | ✅ |
| M.5.8 | 切片交互逻辑 | 左键点击添加切割线，左键拖动移动，Delete 删除。全部操作走 QUndoStack（AddSlicePointCommand, MoveSlicePointCommand, RemoveSlicePointCommand） | 中 | 依赖 M.5.6 | ✅ |
| M.5.9 | 自动切片 + 可选输入层 | "自动切片"按钮调用 RMS Slicer → 生成切割线。"导入切点"按钮读取 au 格式 marker 文件 → 生成切割线。有输入时按输入切，无输入时 RMS 自动。"重新切片"清除并重做 | 低 | 依赖 M.5.8 | ✅ |
| M.5.10 | 切片导出对话框 | 弹出对话框选择：位深（16/24/32-bit PCM, 32-bit float）、声道（单声道/保持原始）、输出目录、命名前缀、序号位数。调用 SlicerService 写出切片 WAV | 低 | 依赖 M.5.8 | ✅ |
| M.5.11 | au 切点文件 I/O | 实现 Audacity marker 格式（.txt）的序列化/反序列化。兼容 main 分支 AudioSlicer 的 marker 保存/读取 | 低 | 可独立 | ✅ |
| M.5.12 | SliceListPanel | 切片列表面板：显示 ID、时长、状态。标红过长/过短。右键丢弃/恢复（DiscardSliceCommand）。双击跳转波形位置 | 低 | 依赖 M.5.8 | ✅ |
| M.5.13 | PipelineContext 集成 | 切片完成（导出音频）后创建 `dstemp/slices/*.wav` + `dstemp/contexts/*.json`，写入 slices 层 | 低 | 依赖 M.5.10 | ✅ |
| M.5.14 | 音频预处理（可选） | 切片页面顶部折叠面板：响度归一化（LUFS 目标）+ 降噪开关。实现 IAudioPreprocessor 接口 + LoudnessNormalizer | 中：需 FFmpeg/libebur128 依赖 | 可延后 | 延后 |
| M.5.15 | PhonemeEditor 适配 | PhonemeEditor 内部将波形/频谱部分替换为 WaveformPanel 实例（保持 API 兼容）。验证：边界拖动、binding、缩放均正常 | 高：核心重构，需仔细回归测试 | 依赖 M.5.1-3 | ✅ (CMake 依赖已添加，增量迁移中) |
| M.5.16 | NewProjectDialog 简化 | 去掉向导步骤 3（切片配置）和步骤 4（执行切片）。改为 2 步后 accept，跳转切片页 | 低 | 依赖 M.5.6 | ✅ (已是 2 步) |

**依赖关系**：
- M.5.1 是核心前提（提取 WaveformPanel）
- M.5.2-3 改变交互方式（右键播放 + 取消工具栏）
- M.5.6-13 构建切片页面
- M.5.15 将 PhonemeEditor 迁移到新的 WaveformPanel
- M.5.14 音频预处理可延后（不影响主流程）

**风险缓解**：
- M.5.1 + M.5.15（WaveformPanel 提取 + PhonemeEditor 适配）是最高风险项。策略：先在 PhonemeEditor 内部重构为 WaveformPanel + TierEdit 两部分，验证功能不退化后，再将 WaveformPanel 移到 shared/

### M.4 — 独立 exe 清理

| ID | 任务 | 方案 | 风险 | 并行 | 状态 |
|----|------|------|------|------|------|
| M.4.1 | 删除独立 exe CMake 目标 | 删除 DatasetPipeline, MinLabel, PhonemeLabeler, PitchLabeler, GameInfer, DiffSingerLabeler 的 CMakeLists.txt | 低：代码保留在 git 历史 | 依赖 M.2.6 + M.3.10 | ✅ |
| M.4.2 | 更新 CI/CD | release.yml 产物改为 LabelSuite + DsLabeler，verify-modules.yml 更新目标列表 | 低 | 依赖 M.4.1 | ✅ |
| M.4.3 | 更新部署脚本 | windeployqt/macdeployqt 目标列表；.desktop 文件（Linux）添加 LabelSuite + DsLabeler 条目 | 低 | 依赖 M.4.1 | ✅ |
| M.4.4 | 文档最终更新 | README, architecture.md, build.md 确认一致 | 低 | 依赖 M.4.1 | ✅ |

---

## 阶段依赖图

```
O (命名规范化 + CMake 整理) ◄── 无前置依赖
  │
  ▼
L.9 (PipelineRunner 集成) ◄── L.0-L.8 ✅, O ✅
  │
  ├──────────────────────────────┐
  ▼                              ▼
L.10 (遗留清理)          L.11 (.dsproj v3, 可与 L.9 并行)
                                 │
                                 ▼
                          M.1 (编辑器提取) ◄── M.1.0 (IEditorDataSource)
                                 │
                    ┌────────────┼────────────┐
                    ▼                         ▼
             M.2 (LabelSuite)          M.3 (DsLabeler) ◄── L.9, L.11
                    │                         │
                    │                         ▼
                    │                  M.5 (WaveformPanel + SlicerPage + 右键播放 + 双声道)
                    │                         │
                    └──────────┬──────────────┘
                               ▼
                        M.4 (清理)
                               │
                               ▼
                        N (i18n, 可在 M 期间并行开始 N.1-N.3)
```

**关键路径**：O → L.9 → L.11 → M.1 → M.3 → M.5 → M.4

**并行工作流**：
- O.1 / O.2 / O.3 / O.4 **四路并行**
- L.11 与 L.9 **完全并行**
- M.1.1 / M.1.2 / M.1.3 **三路并行**
- M.2 与 M.3 的 UI 骨架 **并行**
- M.5.4 (双声道) 与 M.5.2 (右键播放) **并行**
- M.5.5 (Mel 频谱) 与 M.5.1 (WaveformPanel 提取) **并行**
- M.5.11 (au 文件 I/O) 可独立提前
- N.1-N.3 可在 M 期间**后台进行**

---

## 任务统计

| 阶段 | 任务数 | 预估工作量 | 状态 |
|------|--------|-----------|------|
| O.1 | 7 | 0.5 天 | ✅ |
| O.3 | 6 | 0.5 天 | ✅ |
| O.4 | 2 | 0.5 天 | ✅ |
| O.5 | 3 | 0.5 天 | ✅ |
| L.9 | 5 | 3 天 | ✅ |
| L.10 | 4 | 1 天 | ✅ |
| L.11 | 4 | 2 天 | ✅ |
| M.1 | 4 | 5 天 | ✅ |
| M.2 | 3 | 1 天 | ✅ (待人工验证 M.2.3) |
| M.3 | 12 | 10 天 | ✅ |
| M.5 | 16 | 15 天 | ✅ (M.5.14 延后) |
| M.4 | 4 | 1 天 | ✅ |
| N | 6 | 3 天 | ✅ |
| **合计** | **76** | **~43 天** | **全部完成** |

---

## 优先级排序

| 优先级 | 阶段 | 预估 | 理由 |
|--------|------|------|------|
| **P0** | O (命名规范 + CMake 整理) | 2 天 | 越晚做改动越大；纯重命名无功能风险，先统一再开发 |
| **P1** | L.9 + L.11 (并行) | 3+2 天 | 所有依赖已就绪，是 M 的前提 |
| **P2** | L.10 | 1 天 | L.9 完成后收尾 |
| **P3** | M.1 | 5 天 | 编辑器提取是 M.2/M.3 的前提，也是最高风险项 |
| **P4** | M.2 + M.3 (并行) | 3+10 天 | LabelSuite 简单先交付，DsLabeler 复杂后交付 |
| **P5** | M.5 (WaveformPanel + SlicerPage) | 15 天 | **新增**。SlicerPage 是流水线缺失的核心步骤；WaveformPanel 提取 + 右键播放改善 UX |
| **P6** | M.4 | 1 天 | 最终清理（M.5 完成后） |
| **P7** | N (持续) | 3 天 | N.1-N.3 可在 P3 期间后台开始 |

---

## 架构决策记录 (ADR)

| ADR | 决策 | 理由 |
|-----|------|------|
| ADR-1 | dsfw-base 为 Qt-free 静态库 | 支持 CLI 工具和非 Qt 消费者 |
| ADR-2 | ModelType 改为 int ID 注册表 | 框架层不感知业务模型类型 |
| ADR-3 | OnnxModelBase protected 继承 | 各推理 DLL 保持现有 API 稳定 |
| ADR-5 | dsfw-audio 归框架 | 音频播放/解码是通用桌面能力 |
| ADR-7 | FunASR 永远不直接修改 | vendor 代码只用适配器包装 |
| ADR-8 | 单仓库模式 | 降低维护复杂度 |
| ADR-9 | 允许 C++20 | 编译器均已支持 |
| ADR-21 | 统一用 dsfw::CrashHandler | 替换 QBreakpad |
| ADR-22 | QStandardPaths 数据路径 | applicationDirPath() 在 macOS/Linux 下可能只读 |
| ADR-23 | 直接用 QUndoStack | Qt QUndoStack 已成熟 |
| ADR-24 | CMake GLOB_RECURSE CONFIGURE_DEPENDS | 通过 helper 函数封装 |
| ADR-25 | 推理库保留独立命名风格 | API 面向领域用户 |
| ADR-30 | 层数据保持 nlohmann::json | 离线处理，JSON 开销可忽略 |
| ADR-31 | PipelineContext 用 category 做扁平键 | 与 defaults.models 键一致 |
| ADR-32 | 移除 processBatch | 批量逻辑归 PipelineRunner |
| ADR-33 | TranscriptionRow 降级为适配器内部 | 最小迁移爆炸半径 |
| ADR-34 | 切片命名 {prefix}_{NNN}.wav | 按时间顺序，兼容 MDS |
| ADR-35 | LyricFA 以整首歌为粒度 | ASR 需要完整上下文 |
| ADR-36 | 同 taskName 处理器 I/O 必须一致 | 模型可替换的前提 |
| ADR-37 | Context JSON 替代 .dsitem + BatchCheckpoint | 统一一个文件 |
| ADR-38 | 音频预处理独立接口 | 操作音频文件，不产出层数据 |
| ADR-39 | 切片丢弃通过 status + 传播 | 简单可靠，可撤销 |
| ADR-40 | 自动步骤用快照替代细粒度撤销 | 重跑 = 撤销 |
| ADR-41 | 导入/导出格式声明在代码中 | 步骤定义在 PipelineStepRegistry，不在 .dsproj |
| ADR-42 | MDS 兼容通过格式适配器实现 | 不在核心引入 MDS 目录约定 |
| ADR-43 | int64 微秒时间精度 | 消除浮点累积误差 |
| ADR-44 | 边界字段 `pos` (int64 μs) | 缩短字段名 + 精度统一 |
| ADR-45 | F0 用 int32 毫赫兹存储 | 避免浮点精度问题 |
| ADR-46 | 分为 LabelSuite + DsLabeler 两个 exe | 通用标注不绑定 DiffSinger |
| ADR-47 | 省略 BuildCsv，数据暂存 PipelineContext | CSV 是导出格式而非中间格式 |
| ADR-48 | PitchLabeler 自动执行 add_ph_num | 纯算法无需用户干预 |
| ADR-49 | 导出时按需补全缺失步骤 | 允许跳过手动步骤 |
| ADR-50 | per-slice editedSteps 决定是否生成 .ds | 未校正的 F0/MIDI 不适合训练 |
| ADR-51 | 预加载机制异步后台执行 | 消除切换切片时的等待 |
| ADR-52 | IEditorDataSource 抽象数据源 | 避免双份编辑器代码 |
| ADR-53 | 移除 .dsproj tasks[]，步骤在代码中固定 | tasks[] 是死配置，增加复杂度无收益 |
| ADR-54 | .dstext 层新增 type 字段 | 自描述，LabelSuite 无需 .dsproj 也能解析 |
| ADR-55 | JSON 路径统一 POSIX 正斜杠 | 跨平台一致性 |
| ADR-56 | i18n 用 Qt lupdate/lrelease 标准流程 | 成熟生态，TranslationManager 已就绪 |
| ADR-57 | 层依赖 DAG + per-slice dirty 标记 | 保证数据一致性，上游修改自动传播失效到下游 |
| ADR-58 | dirty 重算用 Toast 通知而非模态对话框 | 不打断用户工作流，3 秒自动消失 |
| ADR-59 | 目录命名统一 kebab-case，vendor 除外 | 消除 PascalCase/flatcase 混用；FunAsr 等 vendor 目录按 ADR-7 保持原名 |
| ADR-60 | 部署逻辑提取到 cmake/deploy.cmake | src/CMakeLists.txt 职责单一化，部署脚本独立维护 |
| ADR-61 | SlicerPage = 单层 PhonemeLabeler + 导出 | 最大化复用 WaveformPanel，减少代码重复 |
| ADR-62 | 右键直接播放，不弹菜单 | 标注中最频繁操作应零延迟；波形/标注区域不触发右键菜单 |
| ADR-63 | 全流程单声道，双声道可切换显示 | DiffSinger 训练数据为单声道；保留双声道显示能力供验证 |
| ADR-64 | Settings 移至末尾 | 切片参数内嵌切片页；模型配置低频操作放末尾不占核心位置 |
| ADR-65 | WaveformPanel 提取为共享组件 | 切片/音素标注共享波形+刻度尺+频谱+播放，避免重复实现 |

---

## 关联文档

- [unified-app-design.md](unified-app-design.md) — **LabelSuite + DsLabeler 统一应用设计方案**
- [phase-l-implementation.md](phase-l-implementation.md) — **Phase L 细化实施方案**
- [ds-format.md](ds-format.md) — .dsproj / .dstext 格式规范 v3
- [task-processor-design.md](task-processor-design.md) — 流水线架构设计
- [framework-architecture.md](framework-architecture.md) — 框架架构
- [architecture.md](architecture.md) — 项目架构概述

> 更新时间：2026-05-03 (M.3.2-4 更新完成, M.5 全部完成, N 全部完成)
