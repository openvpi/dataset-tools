# 架构重构路线图

> 基于 2026-05-03 更新。
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
| L | 层路由流水线 | TimePos, CurveTools, Boundary/Layer, PipelineContext, PipelineRunner, FormatAdapters, F0/PhonemeLabeler 迁移, DiffSingerLabeler 集成, 遗留清理, .dsproj v3, 编译速度优化 |
| O | 命名规范化 + CMake 整理 | 目录 kebab-case 统一，deploy.cmake 提取，代码清理 |
| M.1 | 核心编辑器提取 | IEditorDataSource 接口，MinLabelEditor/PhonemeEditor/PitchEditor 提取到 shared/ |
| M.2 | LabelSuite | AppShell 9 页面注册，File 菜单仅保留 Set Working Directory / Clean / Exit |
| M.3 | DsLabeler | 7 页面 AppShell（Welcome, Slicer, MinLabel, PhonemeLabeler, PitchLabeler, Export, Settings），ProjectDataSource，层依赖失效，dirty 自动重算 |
| M.4 | 独立 exe 清理 | 删除独立 exe CMake 目标，CI/CD 更新，部署脚本更新 |
| M.5 | WaveformPanel + SlicerPage | WaveformPanel 共享组件，右键播放，双声道，DsSlicerPage，SliceNumberLayer，切片交互，自动切片，导出，au 文件 I/O，SliceListPanel，MelSpectrogramWidget（M.5.14 音频预处理延后） |
| N | 国际化 (i18n) | Qt lupdate/lrelease，中英双语翻译，语言切换 UI，CI 翻译检查 |

**全部 76 项任务已完成**（M.5.14 音频预处理延后，不影响主流程）。

---

## Phase P — LabelSuite ↔ DsLabeler 高度统一

> **目标**：分两步进行。第一步（P.A）DsLabeler 参照 LabelSuite 补齐页面功能；第二步（P.B）统一底层数据模型和页面代码。P.A 完成后需手动确认才能开始 P.B。

---

### Phase P.A — DsLabeler 页面功能补齐（参照 LabelSuite）

> **前置条件**：无。
> **完成标准**：DsLabeler 各页面功能与 LabelSuite 对应页面对齐，手动确认后进入 P.B。

#### P.A.1 DsMinLabelPage 功能补齐

参照 `MinLabelPage`（LabelSuite）补齐：
- [ ] 快捷键管理（ShortcutManager 集成）
- [ ] 编辑菜单（Undo/Redo、G2P 转换、导出等）
- [ ] 文件状态委托（FileStatusDelegate — 标注完成状态显示）
- [ ] 导航快捷键（上一个/下一个切片）
- [ ] 播放快捷键
- [ ] 状态栏内容（进度信息）
- [ ] 拖拽支持（如有意义）

#### P.A.2 DsPhonemeLabelerPage 功能补齐

参照 `PhonemeLabelerPage`（LabelSuite）补齐：
- [ ] 自动 FA 实际调用 HubertFA 推理（当前为 stub）
- [ ] 批量 FA 实际调用推理
- [ ] 快捷键管理（ShortcutManager）
- [ ] 完整编辑菜单（已有 Undo/Redo，检查是否缺其他操作）
- [ ] 文件导航（上一个/下一个切片快捷键）
- [ ] 工具栏集成到页面（当前 `toolbar()` 暴露但未确认集成）
- [ ] 右键播放行为验证（ADR-62）

#### P.A.3 DsPitchLabelerPage 功能补齐

参照 `PitchLabelerPage`（LabelSuite）补齐：
- [ ] 自动 F0 提取实际调用 RMVPE（当前为 stub）
- [ ] 自动 MIDI 转录实际调用 GAME（当前为 stub）
- [ ] 自动 add_ph_num 执行（当前无实现）
- [ ] 批量提取实际调用推理
- [ ] 快捷键管理（ShortcutManager）
- [ ] 文件导航（上一个/下一个切片快捷键）
- [ ] A/B 比较功能（abCompareAction）
- [ ] 缩放操作（zoomIn/Out/Reset — 已有菜单项，确认功能正常）
- [ ] 完整状态栏信息

#### P.A.4 DsSlicerPage 功能验证

参照 `SlicerPage`（LabelSuite pipeline）验证：
- [ ] 自动切片算法调用正确
- [ ] 批量导出所有切片音频
- [ ] 导入/保存 Audacity 标记文件
- [ ] 切片参数持久化到工程（.dsproj defaults.slicer）
- [ ] 切点变更提醒（已有切片的重新切片警告）
- [ ] 进度条更新

#### P.A.5 SettingsPage 功能补齐

- [ ] 设备 tab（GPU 枚举、推理提供者选择）
- [ ] ASR tab（模型路径 + Test 按钮 + CPU 强制）
- [ ] 强制对齐 tab（模型路径 + Test + 预加载）
- [ ] 音高/MIDI tab（模型路径 + Test + 预加载）
- [ ] 词典/G2P tab
- [ ] 配置变更触发 modelReloadRequested 信号

#### P.A.6 推理库集成

DsPhonemeLabelerPage 和 DsPitchLabelerPage 的核心功能依赖推理库实际调用：
- [ ] HubertFA 推理集成（DsPhonemeLabelerPage::onRunFA 实现）
- [ ] RMVPE 推理集成（DsPitchLabelerPage::onExtractPitch 实现）
- [ ] GAME 推理集成（DsPitchLabelerPage::onExtractMidi 实现）
- [ ] add_ph_num 算法集成
- [ ] 预加载机制实现（Settings 中配置的前 N 个文件后台执行）

#### P.A.7 ExportPage 实现

- [ ] 导出前置校验（grapheme 层检查）
- [ ] 自动补全缺失步骤（FA/add_ph_num/GAME/RMVPE）
- [ ] CSV 生成（CsvAdapter）
- [ ] .ds 文件生成（per-slice editedSteps 判定）
- [ ] wavs/ 导出（可选重采样）
- [ ] 进度显示

---

### ⏸ 手动确认节点

> Phase P.A 完成后，需手动确认 DsLabeler 各页面功能与 LabelSuite 一致，方可开始 P.B。

---

### Phase P.B — 统一底层数据模型和页面代码

> **前置条件**：Phase P.A 手动确认通过。
> **完成标准**：两个应用共享全部页面组件和 dstext 数据模型。

#### P.B.1 ISettingsBackend 抽象 + SettingsWidget 共享

- [ ] 提取 `ISettingsBackend` 接口（`load()` / `save()` 返回 `QJsonObject`）
- [ ] 实现 `AppSettingsBackend`（基于 `QSettings`，LabelSuite 用）
- [ ] 实现 `ProjectSettingsBackend`（基于 `.dsproj` defaults，DsLabeler 用）
- [ ] 提取 SettingsWidget 到 `src/apps/shared/settings/`
- [ ] 两个应用共享 SettingsWidget

#### P.B.2 FileDataSource 改造为 dstext 内核

- [ ] `FileDataSource` 内部使用 `DsTextDocument`
- [ ] `loadSlice()` 通过 FormatAdapter 从旧格式导入
- [ ] `saveSlice()` 通过 FormatAdapter 导出回原格式
- [ ] 支持格式：TextGrid、.lab、.ds、CSV
- [ ] 移动 FileDataSource 到 `src/apps/shared/data-sources/`

#### P.B.3 页面代码统一

- [ ] 移除 LabelSuite 独有的 `TaskWindowAdapter`
- [ ] 统一 MinLabelPage / DsMinLabelPage 为单一实现 + 数据源注入
- [ ] 统一 PhonemeLabelerPage / DsPhonemeLabelerPage 为单一实现
- [ ] 统一 PitchLabelerPage / DsPitchLabelerPage 为单一实现
- [ ] LabelSuite 专有页面（ASR、Align、CSV、MIDI、DS）改用 dstext 底层

#### P.B.4 LabelSuite 自动补全

- [ ] Phone 页进入时自动 FA（与 DsLabeler 一致）
- [ ] Pitch 页进入时自动 add_ph_num + F0 + MIDI
- [ ] Toast 通知行为统一
- [ ] Align/MIDI 独立页面保留

#### P.B.5 LabelSuite 增加 Settings 页

- [ ] 添加第 10 个页面：Settings
- [ ] 使用共享 SettingsWidget + AppSettingsBackend
- [ ] 侧边栏图标

#### P.B.6 旧代码清理

- [ ] 删除 `src/apps/label-suite/pages/` 独立实现（改用 shared 版本）
- [ ] 更新 LabelSuite CMakeLists.txt
- [ ] 确保两个应用 CMake 依赖集一致化

#### P.B.7 集成测试 + 旧格式兼容验证

- [ ] LabelSuite 打开/编辑/保存 TextGrid 文件
- [ ] LabelSuite 打开/编辑/保存 .ds 文件
- [ ] LabelSuite 打开/编辑/保存 .lab 文件
- [ ] 自动补全行为验证
- [ ] Settings 持久化验证
- [ ] DsLabeler 回归测试

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
| ADR-66 | LabelSuite 底层统一使用 dstext/PipelineContext | 消除数据模型分裂；旧格式通过 FormatAdapter 兼容 |
| ADR-67 | LabelSuite 增加 Settings 页 | 统一模型配置体验；共享 SettingsWidget |
| ADR-68 | LabelSuite 享受自动补全 | 与 DsLabeler 一致；同时保留独立页面供手动控制 |
| ADR-69 | ISettingsBackend 抽象持久化 | Settings UI 共享，后端可切换 QSettings / .dsproj |
| ADR-70 | FileDataSource 内部使用 dstext + FormatAdapter | 旧格式文件自动转入/转出 dstext，用户无感知 |

---

## 关联文档

- [unified-app-design.md](unified-app-design.md) — **LabelSuite + DsLabeler 统一应用设计方案**
- [phase-l-implementation.md](phase-l-implementation.md) — **Phase L 细化实施方案**
- [ds-format.md](ds-format.md) — .dsproj / .dstext 格式规范 v3
- [task-processor-design.md](task-processor-design.md) — 流水线架构设计
- [framework-architecture.md](framework-architecture.md) — 框架架构
- [architecture.md](architecture.md) — 项目架构概述

> 更新时间：2026-05-03 — 新增 Phase P（LabelSuite ↔ DsLabeler 高度统一）
