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

> 更新时间：2026-05-03
