# dataset-tools 大型重构方案 v5.1

> 版本：5.1.0
> 日期：2026-06-06
> 状态：规划中
> 范围：目录结构重整、命名空间统一、CMake 优化、安全加固、可扩展性、可读性
> 原则：优先保证稳定性，每阶段自包含可编译

---

## 0. 核心原则

### 0.1 功能完全不变
所有对外行为保持一致。重构仅改变内部组织方式，不改变可观测行为。

### 0.2 逐阶段编译验证
每个阶段完成后必须通过全量编译（`cmake --build --preset release`），确保无回归。

### 0.3 设计准则优先
所有方案必须通过 [human-decisions.md](../../human-decisions.md) 速查表逐条复核。冲突时以设计准则为准。

### 0.4 最小变更范围
每个 PR/提交仅涉及一个阶段的变更，变更范围可控、可回滚。

### 0.5 安全第一
涉及文件 I/O、配置管理、用户输入的所有变更必须通过安全审查（详见 Phase 9-10）。

### 0.6 可扩展性内建
新增接口/注册机制必须遵循开闭原则（详见 Phase 11-12）。

### 0.7 可读性一致
所有命名、目录、include 路径必须遵循统一规范（详见 Phase 13-14）。

---

## 1. 现状诊断

### 1.1 目录结构问题

| 编号 | 问题 | 位置 | 严重度 |
|------|------|------|--------|
| D1 | signal 头文件位置错误 | `src/framework/core/include/dsfw/signal/` 下有 curve_tools.h、music_math.h、time_series.h，但实现文件在 `src/framework/signal/src/` | 🔴 高 |
| D2 | infer 头文件命名空间混乱 | `src/framework/infer/include/dstools/` 下有 OnnxEnv.h、OnnxModelBase.h、InferenceModelProvider.h（应为 dsfw::infer），以及 IInferenceEngine.h 向后兼容 shim | 🔴 高 |
| D3 | audio 目录多余嵌套 | `src/framework/audio/dsfw-audio/` 仅有一个子目录，应扁平化为 `src/framework/audio/` | 🟡 中 |
| D4 | types 不在 framework 下 | `src/types/` 是 header-only 基础类型库，逻辑上属于框架层，但目录在 `src/` 顶层 | 🟡 中 |
| D5 | ui-core 不在 framework 下 | `src/ui-core/` 是 dstools-ui-core（应用层包装），但 `src/framework/ui-core/` 是 dsfw-ui-core（框架层），两个 ui-core 容易混淆 | 🟡 中 |
| D6 | infer/libs 与 framework 分离 | `src/infer/`（推理引擎）和 `src/libs/`（引擎适配器）在顶层，与 `src/framework/` 同级，但逻辑上 libs 是应用层代码 | 🟢 低 |
| D7 | apps/shared 子模块命名不统一 | `data-sources`、`chart-framework`、`audio-visualizer` 等命名风格不一致 | 🟢 低 |

### 1.2 命名空间问题

| 编号 | 问题 | 位置 | 严重度 |
|------|------|------|--------|
| N1 | `dsfw::music` 命名空间 | `src/framework/core/include/dsfw/signal/music_math.h` — 音乐数学函数使用了独立命名空间，应统一为 `dsfw::signal` | 🟡 中 |
| N2 | `dstools::labeler` 命名空间 | `src/framework/ui-core/include/dsfw/IPageActions.h`、`IPageLifecycle.h` — 框架接口使用了应用层命名空间 | 🟡 中 |
| N3 | `chart` 命名空间 | `src/apps/shared/chart-framework/ChartConfigRegistry.h` — 未使用 `dstools` 前缀 | 🟢 低 |
| N4 | `InferBridge` 命名空间 | `src/libs/infer-bridge/InferBridge.h` — 未使用 `dstools` 前缀 | 🟢 低 |
| N5 | 框架头文件中的 `namespace dstools` alias | 多处框架头文件末尾有 `namespace dstools { using dsfw::X; }` 向后兼容别名 — 应在 v5 中清理 | 🟡 中 |
| N6 | `dstools::infer` 向后兼容 shim | `src/framework/infer/include/dstools/IInferenceEngine.h` — 仅包含 using 声明，应删除 | 🟢 低 |

### 1.3 CMake 构建系统问题

| 编号 | 问题 | 位置 | 严重度 |
|------|------|------|--------|
| C1 | dsfw-audio 缺少 NAMESPACE 别名 | `src/framework/audio/dsfw-audio/CMakeLists.txt` — 无 `NAMESPACE dsfw::audio` | 🟡 中 |
| C2 | infer-common 缺少 NAMESPACE 别名 | `src/framework/infer/CMakeLists.txt` — 无 NAMESPACE | 🟡 中 |
| C3 | src/CMakeLists.txt 目录分散 | types、ui-core、infer、libs 与 framework 同级，逻辑分组不清晰 | 🟡 中 |
| C4 | apps/shared 子模块 CMake target 命名不统一 | `data-sources`、`chart-framework`、`audio-visualizer` 等 target 名与目录名一致但风格各异 | 🟢 低 |
| C5 | 缺少 install(TARGETS) 规则 | 部分库仅有 EXPORT 导出，缺少 TARGETS 安装规则 | 🟢 低 |
| C6 | dstools-types namespace 命名不直观 | `dstools-types::dstools-types` 双重命名，应简化为 `dsfw::types` | 🟡 中 |
| C7 | vcpkg manifest 位置非标准 | `scripts/vcpkg-manifest/vcpkg.json` 不在项目根目录，IDE 无法自动识别 | 🟡 中 |
| C8 | 缺少 CMakePresets.json | 无预设配置，开发者需手动指定编译参数 | 🟡 中 |
| C9 | 缺少 version.h 生成 | 各应用无统一版本号头文件，版本信息在 CMake 中硬编码 | 🟢 低 |
| C10 | 预编译头使用不一致 | 仅部分模块使用 PCH，部分模块未启用 | 🟢 低 |

### 1.4 安全问题

| 编号 | 问题 | 位置 | 严重度 |
|------|------|------|--------|
| S1 | 缺少路径遍历防护 | 所有接受用户文件路径的接口（文件导入、格式适配器）未做路径逃逸检测 | 🔴 高 |
| S2 | 配置加载无完整性校验 | 配置文件加载后未验证 JSON schema 完整性，损坏的配置可能导致未定义行为 | 🔴 高 |
| S3 | 缺少配置版本迁移策略 | 配置文件格式变更后，旧版本配置无法自动迁移，用户需手动删除重建 | 🟡 中 |
| S4 | 缺少输入大小限制 | 格式适配器、推理引擎的输入数据大小无上限控制，可能导致内存耗尽 | 🟡 中 |
| S5 | 临时文件未清理 | PipelineRunner 的快照文件、部分中间文件可能在异常退出后残留 | 🟢 低 |
| S6 | 日志文件无大小限制 | 无日志轮转策略，长期运行可能填满磁盘 | 🟢 低 |

### 1.5 可扩展性问题

| 编号 | 问题 | 位置 | 严重度 |
|------|------|------|--------|
| E1 | 格式适配器注册机制分散 | IFormatAdapter 接口存在，但注册通过 FormatAdapterRegistry 手动管理，新增格式需修改多处代码 | 🟡 中 |
| E2 | 推理引擎集成无统一模式 | 各推理引擎的 CMake 配置、接口适配、错误处理方式各不相同 | 🟡 中 |
| E3 | 缺少插件发现机制 | 新增页面/图表/工具需修改 CMake 和代码注册，无动态加载能力 | 🟢 低 |
| E4 | 配置文件 schema 未版本化 | DsProject 的 JSON 格式无 schema 版本字段，前后兼容性依赖手动处理 | 🟡 中 |
| E5 | 任务处理器注册不透明 | TaskProcessorRegistry 新增处理器需修改多处，缺少自注册模式 | 🟢 低 |

### 1.6 可读性问题

| 编号 | 问题 | 位置 | 严重度 |
|------|------|------|--------|
| R1 | include 路径约定不一致 | 部分模块使用 `#include <dsfw/X.h>`，部分使用 `#include <dstools/X.h>`，部分使用裸文件名 | 🟡 中 |
| R2 | 头文件命名风格不统一 | 框架层头文件命名：PascalCase（PathUtils.h）vs snake_case（curve_tools.h） | 🟡 中 |
| R3 | PIMPL 使用不一致 | DsProject 使用 PIMPL，但部分同等复杂度的类未使用，导致头文件暴露实现细节 | 🟢 低 |
| R4 | 前向声明不充分 | 部分头文件包含不必要的完整头文件，可用前向声明替代，增加编译依赖 | 🟢 低 |
| R5 | 注释语言混用 | 部分注释为中文，部分为英文，部分为 Doxygen 格式，部分为普通注释 | 🟢 低 |
| R6 | `dstools_add_library` 自动 GLOB 不可控 | 若 include/ 目录下存在非公共头文件，也会被导出，可能导致符号冲突 | 🟡 中 |

### 1.7 对照设计准则诊断

| 问题 | 违反准则 | 说明 |
|------|---------|------|
| 框架层使用 `dstools::labeler` 命名空间 | ARCH-03（接口稳定） | 框架接口应在 `dsfw` 命名空间下 |
| signal 头文件在 core 目录 | ARCH-01（职责单一） | core 模块不应包含 signal 模块的公共头文件 |
| infer 头文件在 dstools 目录 | ARCH-06（依赖倒置） | 框架层代码不应放在应用层命名空间目录下 |
| 缺少路径遍历防护 | ROBUST-01（输入验证） | 所有外部输入必须在边界处验证 |
| 格式适配器注册分散 | ARCH-09（开闭原则） | 新增格式应只需添加新类，不修改核心代码 |
| 配置文件无版本化 | ROBUST-02（向前兼容） | 数据格式应支持版本化迁移 |

---

## 2. 目标架构

### 2.1 目标目录树

```
src/
├── framework/                          # dsfw 通用框架
│   ├── CMakeLists.txt                  # 框架层总控
│   ├── types/                          # dsfw-types (header-only, 从 src/types/ 移入)
│   │   ├── include/dsfw/
│   │   │   ├── Result.h
│   │   │   ├── ErrorCode.h
│   │   │   ├── TimePos.h
│   │   │   └── infer/
│   │   │       └── ExecutionProvider.h
│   │   └── CMakeLists.txt
│   ├── base/                           # dsfw-base (Qt-free)
│   │   ├── include/dsfw/
│   │   │   └── * (JsonHelper 等)
│   │   └── CMakeLists.txt
│   ├── core/                           # dsfw-core (Qt Core)
│   │   ├── include/dsfw/
│   │   │   ├── AppSettings.h
│   │   │   ├── PathUtils.h
│   │   │   ├── ServiceLocator.h
│   │   │   ├── IFormatAdapter.h
│   │   │   ├── ITaskProcessor.h
│   │   │   ├── PipelineRunner.h
│   │   │   ├── AsyncTask.h
│   │   │   ├── AtomicFileWriter.h
│   │   │   └── ... (接口/工具类)
│   │   ├── src/
│   │   └── CMakeLists.txt
│   ├── signal/                         # dsfw-signal (信号处理)
│   │   ├── include/dsfw/signal/
│   │   │   ├── CurveTools.h           # ← 从 core/include 移入 + 重命名
│   │   │   ├── MusicMath.h            # ← 从 core/include 移入 + 重命名
│   │   │   ├── TimeSeries.h           # ← 从 core/include 移入 + 重命名
│   │   │   └── Slicer.h
│   │   ├── src/
│   │   │   ├── CurveTools.cpp
│   │   │   ├── Slicer.cpp
│   │   │   └── MathUtils.h
│   │   └── CMakeLists.txt
│   ├── infer/                          # dsfw-infer (推理公共)
│   │   ├── include/dsfw/infer/
│   │   │   ├── IInferenceEngine.h
│   │   │   ├── IInferenceService.h
│   │   │   ├── OnnxEnv.h              # ← 从 dstools/ 移入
│   │   │   ├── OnnxModelBase.h        # ← 从 dstools/ 移入
│   │   │   └── InferenceModelProvider.h # ← 从 dstools/ 移入
│   │   ├── src/
│   │   │   ├── OnnxEnv.cpp
│   │   │   └── OnnxModelBase.cpp
│   │   └── CMakeLists.txt
│   ├── audio/                          # dsfw-audio (扁平化)
│   │   ├── include/dsfw/audio/
│   │   │   ├── AudioBuffer.h
│   │   │   ├── FfmpegAudioDecoder.h
│   │   │   ├── AudioPipeline.h
│   │   │   └── ...
│   │   ├── src/
│   │   ├── tests/
│   │   └── CMakeLists.txt
│   ├── ui-core/                        # dsfw-ui-core (AppShell)
│   │   ├── include/dsfw/
│   │   │   ├── AppShell.h
│   │   │   ├── IPageActions.h          # 命名空间改为 dsfw
│   │   │   ├── IPageLifecycle.h        # 命名空间改为 dsfw
│   │   │   ├── Theme.h
│   │   │   └── ...
│   │   ├── src/
│   │   └── CMakeLists.txt
│   └── widgets/                        # dsfw-widgets (SHARED)
│       ├── include/dsfw/widgets/
│       │   ├── ShortcutManager.h
│       │   ├── PlayWidget.h
│       │   ├── PathSelector.h
│       │   └── ...
│       ├── src/
│       └── CMakeLists.txt
│
├── domain/                             # dstools-domain (领域层)
│   ├── include/dstools/
│   │   ├── DsDocument.h
│   │   ├── DsProject.h
│   │   ├── CurveTools.h               # re-export 别名
│   │   ├── F0Curve.h                  # re-export 别名
│   │   ├── MouthCurve.h               # re-export 别名
│   │   └── ...
│   ├── src/
│   └── CMakeLists.txt
│
├── ui-core/                            # dstools-ui-core (应用层共享 UI)
│   ├── include/dstools/
│   │   └── AppInit.h
│   ├── src/
│   └── CMakeLists.txt
│
├── infer/                              # 推理引擎实现（第三方）
│   ├── game-infer/
│   ├── hubert-infer/
│   ├── rmvpe-infer/
│   ├── moe-infer/
│   ├── FunAsr/
│   ├── onnxruntime/
│   └── CMakeLists.txt
│
├── libs/                               # 推理引擎适配器（应用层）
│   ├── game-infer-lib/
│   ├── hubert-fa/
│   ├── lyric-fa/
│   ├── rmvpe-pitch/
│   ├── moe-lib/
│   ├── min-label-lib/
│   ├── slicer/
│   ├── textgrid/
│   ├── infer-bridge/
│   └── CMakeLists.txt
│
├── apps/                               # 应用
│   ├── shared/                         # 应用共享组件
│   │   ├── chart-framework/
│   │   ├── audio-visualizer/
│   │   ├── data-sources/
│   │   ├── phoneme-editor/
│   │   ├── pitch-editor/
│   │   ├── min-label-editor/
│   │   ├── mouth-curve-chart/
│   │   ├── settings/
│   │   ├── bridges/
│   │   ├── log-page/
│   │   └── CMakeLists.txt
│   ├── ds-labeler/
│   ├── label-suite/
│   ├── cli/
│   ├── widget-gallery/
│   ├── unified-editor/
│   └── CMakeLists.txt
│
├── tests/                              # 测试
│   ├── framework/
│   ├── domain/
│   ├── libs/
│   ├── types/
│   ├── widgets/
│   └── CMakeLists.txt
│
└── CMakeLists.txt
```

### 2.2 目标命名空间

| 命名空间 | 所属层 | 说明 |
|---------|--------|------|
| `dsfw` | 框架层 | 通用工具类（PathUtils、AppSettings、ServiceLocator 等） |
| `dsfw::signal` | 框架层 | 信号处理（CurveTools、Slicer、MusicMath、TimeSeries） |
| `dsfw::infer` | 框架层 | 推理接口与公共实现（IInferenceEngine、OnnxEnv、OnnxModelBase） |
| `dsfw::audio` | 框架层 | 音频处理（AudioPipeline、FfmpegAudioDecoder、AudioPlaybackAdapter） |
| `dsfw::widgets` | 框架层 | 通用 UI 组件（PlayWidget、PathSelector、ViewportController 等） |
| `dsfw::CommonKeys` | 框架层 | 框架层配置键 |
| `dstools` | 应用层 | 领域模型、适配器、页面（DsDocument、DsProject、ExportFormats 等） |
| `dstools::settings` | 应用层 | 应用设置键 |
| `dstools::settings::pitch` | 应用层 | 音高编辑器设置键 |
| `dstools::settings::app` | 应用层 | 应用全局设置键 |
| `dstools::settings::inference` | 应用层 | 推理设置键 |
| `Game` | 推理引擎 | game-infer 引擎（第三方，保持原样） |
| `HFA` | 推理引擎 | hubert-infer 引擎（第三方，保持原样） |
| `Rmvpe` | 推理引擎 | rmvpe-infer 引擎（第三方，保持原样） |
| `Moe` | 推理引擎 | moe-infer 引擎（第三方，保持原样） |
| `FunAsr` | 推理引擎 | FunASR 引擎（第三方，保持原样） |
| `LyricFA` | 适配器 | 歌词对齐适配器（保持原样） |
| `Minlabel` | 适配器 | 最小标签适配器（保持原样） |

### 2.3 目标 CMake Target 命名

| Target | 别名 | 类型 | 说明 |
|--------|------|------|------|
| `dsfw-types` | `dsfw::types` | INTERFACE | 基础类型 |
| `dsfw-base` | `dsfw::base` | STATIC | Qt-free 工具 |
| `dsfw-core` | `dsfw::core` | STATIC | 框架核心 |
| `dsfw-signal` | `dsfw::signal` | STATIC | 信号处理 |
| `dsfw-infer` | `dsfw::infer` | STATIC | 推理公共 |
| `dsfw-audio` | `dsfw::audio` | STATIC | 音频处理 |
| `dsfw-ui-core` | `dsfw::ui-core` | STATIC | UI 框架 |
| `dsfw-widgets` | `dsfw::widgets` | SHARED | 通用组件 |
| `dstools-domain` | `dstools::domain` | STATIC | 领域逻辑 |
| `dstools-ui-core` | `dstools::ui-core` | STATIC | 应用共享 UI |
| `game-infer` | `game-infer::game-infer` | SHARED | GAME 推理 |
| `hubert-infer` | `hubert-infer::hubert-infer` | SHARED | HuBERT 推理 |
| `rmvpe-infer` | `rmvpe-infer::rmvpe-infer` | SHARED | RMVPE 推理 |
| `moe-infer` | `moe-infer::moe-infer` | SHARED | MoE 推理 |
| `fun-asr` | — | STATIC | FunASR 推理 |

### 2.4 目标 include 路径约定

| 层 | 头文件路径 | include 指令 | 说明 |
|----|-----------|-------------|------|
| 框架层 | `<dsfw/Xxx.h>` | `#include <dsfw/PathUtils.h>` | 所有框架公共头文件统一使用 `dsfw/` 前缀 |
| 框架子模块 | `<dsfw/module/Xxx.h>` | `#include <dsfw/signal/CurveTools.h>` | 子模块头文件使用 `dsfw/module/` 前缀 |
| 领域层 | `<dstools/Xxx.h>` | `#include <dstools/DsProject.h>` | 领域层公共头文件使用 `dstools/` 前缀 |
| 应用层 | `<dstools/Xxx.h>` | `#include <dstools/AppInit.h>` | 应用层公共头文件使用 `dstools/` 前缀 |

### 2.5 目标头文件命名约定

| 类型 | 命名风格 | 示例 |
|------|---------|------|
| 接口类 | `I` 前缀 + PascalCase | `IFormatAdapter.h`、`ITaskProcessor.h` |
| 工具类 | PascalCase | `PathUtils.h`、`AppSettings.h`、`ServiceLocator.h` |
| 领域模型 | `Ds` 前缀 + PascalCase | `DsDocument.h`、`DsProject.h` |
| 函数集合 | snake_case | `curve_tools.h`、`music_math.h`（信号处理函数集合保持现有风格） |
| 配置键 | PascalCase | `Keys.h`、`CommonKeys.h` |

---

## 3. 重构阶段

### 阶段依赖图

```
Phase 1  (signal 头文件归位)     ──┐
Phase 2  (infer 命名空间统一)    ──┤
Phase 3  (audio 目录扁平化)      ──┤ 可并行
Phase 8  (apps/shared 清理)      ──┤
Phase 13 (头文件命名统一)        ──┘
        │
Phase 4  (框架命名空间清理)      ← 依赖 1, 2, 3
Phase 9  (安全加固 - 路径)       ← 无前置依赖
        │
Phase 5  (types 移入 framework)  ← 依赖 4
Phase 10 (安全加固 - 配置)       ← 依赖 4
        │
Phase 6  (CMake NAMESPACE 补全)  ← 依赖 1~5
Phase 11 (可扩展性 - 格式适配器) ← 无前置依赖
        │
Phase 7  (CMake 目录组织优化)    ← 依赖 6
Phase 14 (PIMPL + 前向声明)      ← 依赖 4
        │
Phase 12 (可扩展性 - 配置版本化) ← 依赖 10
Phase 15 (CMake 预设 + 版本)     ← 依赖 7
Phase 16 (安全加固 - 日志清理)   ← 无前置依赖
```

---

## 4. Phase 1：signal 头文件归位

> **目标**：将 `src/framework/core/include/dsfw/signal/` 下的三个头文件移入 `src/framework/signal/include/dsfw/signal/`。
> **风险**：低 — 仅移动文件，更新 include 路径。
> **对照准则**：ARCH-01（职责单一）— core 模块不应包含 signal 模块的公共头文件。

### 4.1 现状

`src/framework/core/include/dsfw/signal/` 目录当前包含：

| 文件 | 命名空间 | 说明 |
|------|---------|------|
| `curve_tools.h` | `dsfw::signal` | 曲线工具函数声明 |
| `music_math.h` | `dsfw::music` | 音乐数学函数（MIDI 音符转换等） |
| `time_series.h` | `dsfw::signal` | 时间序列模板类（F0Curve, MouthCurve 基类） |

这些文件通过 `dstools_add_library` 的 `GLOB_RECURSE` 被编译入 dsfw-core，但它们的实现（`curve_tools.cpp`）在 `src/framework/signal/src/`。

`src/framework/signal/include/dsfw/signal/` 当前仅有：
- `Slicer.h` — RMS 音频切片器

### 4.2 方案

1. 将 `curve_tools.h`、`music_math.h`、`time_series.h` 从 `src/framework/core/include/dsfw/signal/` 移动到 `src/framework/signal/include/dsfw/signal/`
2. 将 `music_math.h` 中的 `namespace dsfw::music` 改为 `namespace dsfw::signal`
3. 更新所有引用这些头文件的 `#include` 路径（`<dsfw/signal/xxx.h>` 保持不变，因为 include 搜索路径会自动解析新位置）
4. 删除 `src/framework/core/include/dsfw/signal/` 空目录
5. 验证 dsfw-core 不再通过 GLOB_RECURSE 包含 signal 头文件

### 4.3 修改文件清单

| 操作 | 文件 |
|------|------|
| 移动 | `src/framework/core/include/dsfw/signal/curve_tools.h` → `src/framework/signal/include/dsfw/signal/curve_tools.h` |
| 移动 | `src/framework/core/include/dsfw/signal/music_math.h` → `src/framework/signal/include/dsfw/signal/music_math.h` |
| 移动 | `src/framework/core/include/dsfw/signal/time_series.h` → `src/framework/signal/include/dsfw/signal/time_series.h` |
| 修改 | `music_math.h` — `namespace dsfw::music` → `namespace dsfw::signal` |
| 修改 | 所有使用 `dsfw::music::` 的源文件 → 改为 `dsfw::signal::` |
| 删除 | `src/framework/core/include/dsfw/signal/` 目录 |

### 4.4 验收标准

- `src/framework/core/include/dsfw/signal/` 目录不存在
- 所有三个头文件在 `src/framework/signal/include/dsfw/signal/` 下
- `dsfw::music` 命名空间不再存在
- 编译通过
- 所有现有测试通过

---

## 5. Phase 2：infer 命名空间统一

> **目标**：将 `src/framework/infer/include/dstools/` 下的头文件移入 `dsfw/infer/`，统一命名空间为 `dsfw::infer`。
> **风险**：低 — 仅移动文件，更新 include 路径和命名空间引用。
> **对照准则**：ARCH-06（依赖倒置）— 框架层接口应在 `dsfw` 命名空间下。

### 5.1 现状

`src/framework/infer/include/` 下有两个目录：

**dsfw/infer/**（正确位置）：
- `IInferenceEngine.h` — 推理引擎接口（`namespace dsfw::infer`）
- `IInferenceService.h` — 推理服务接口（`namespace dsfw::infer`）

**dstools/**（应迁移）：
- `IInferenceEngine.h` — 向后兼容 shim（`namespace dstools::infer { using dsfw::infer::IInferenceEngine; }`）
- `OnnxEnv.h` — ONNX 环境管理（`namespace dstools::infer`）
- `OnnxModelBase.h` — ONNX 模型基类（`namespace dstools::infer`）
- `InferenceModelProvider.h` — 推理模型提供者模板（`namespace dsfw` 和 `namespace dstools`）

### 5.2 方案

1. 将 `OnnxEnv.h`、`OnnxModelBase.h`、`InferenceModelProvider.h` 移动到 `dsfw/infer/`
2. 将这三个文件的命名空间从 `dstools::infer` 改为 `dsfw::infer`
3. 在 `dstools/` 目录下创建向后兼容 shim（using 声明），一个版本后移除
4. 更新 `src/framework/infer/src/` 中的实现文件命名空间
5. 更新所有引用者（`src/libs/infer-bridge/`、`src/libs/lyric-fa/` 等）
6. 更新 CMakeLists.txt 中的 include 目录配置

### 5.3 修改文件清单

| 操作 | 文件 |
|------|------|
| 移动 | `include/dstools/OnnxEnv.h` → `include/dsfw/infer/OnnxEnv.h` |
| 移动 | `include/dstools/OnnxModelBase.h` → `include/dsfw/infer/OnnxModelBase.h` |
| 移动 | `include/dstools/InferenceModelProvider.h` → `include/dsfw/infer/InferenceModelProvider.h` |
| 修改 | `OnnxEnv.h` — `namespace dstools::infer` → `namespace dsfw::infer` |
| 修改 | `OnnxModelBase.h` — `namespace dstools::infer` → `namespace dsfw::infer` |
| 修改 | `InferenceModelProvider.h` — `namespace dstools` → `namespace dsfw` |
| 修改 | `src/OnnxEnv.cpp` — 命名空间 |
| 修改 | `src/OnnxModelBase.cpp` — 命名空间 |
| 创建 | `include/dstools/OnnxEnv.h` — 向后兼容 shim |
| 创建 | `include/dstools/OnnxModelBase.h` — 向后兼容 shim |
| 创建 | `include/dstools/InferenceModelProvider.h` — 向后兼容 shim |
| 修改 | 所有 `#include <dstools/OnnxEnv.h>` → `#include <dsfw/infer/OnnxEnv.h>` |

### 5.4 验收标准

- `src/framework/infer/include/dstools/` 下仅保留向后兼容 shim
- 新头文件在 `src/framework/infer/include/dsfw/infer/` 下
- `dstools::infer` 命名空间不再包含框架类定义
- 编译通过
- 所有推理功能正常

---

## 6. Phase 3：audio 目录扁平化

> **目标**：移除 `src/framework/audio/dsfw-audio/` 的多余嵌套层级。
> **风险**：极低 — 纯目录重命名，CMake target 名不变。

### 6.1 现状

```
src/framework/audio/
└── dsfw-audio/              ← 仅有这一个子目录
    ├── include/dsfw/audio/
    ├── src/
    ├── tests/
    └── CMakeLists.txt
```

### 6.2 方案

1. 将 `src/framework/audio/dsfw-audio/` 的内容提升到 `src/framework/audio/`
2. 更新 `src/framework/CMakeLists.txt` 中的 `add_subdirectory(audio/dsfw-audio)` → `add_subdirectory(audio)`
3. CMake target 名保持 `dsfw-audio`，别名保持 `dsfw::audio`

### 6.3 修改文件清单

| 操作 | 文件 |
|------|------|
| 移动 | `dsfw-audio/*` → `audio/*` |
| 修改 | `src/framework/CMakeLists.txt` — `add_subdirectory(audio/dsfw-audio)` → `add_subdirectory(audio)` |
| 删除 | 空目录 `audio/dsfw-audio/` |

### 6.4 验收标准

- `src/framework/audio/` 直接包含 include/src/tests/CMakeLists.txt
- `src/framework/audio/dsfw-audio/` 目录不存在
- 编译通过
- 所有音频功能正常

---

## 7. Phase 4：框架命名空间清理

> **目标**：清理框架层头文件中的 `namespace dstools` 向后兼容别名，统一框架层命名空间为 `dsfw`。
> **风险**：中 — 涉及大量文件变更，但每个变更都是机械性的。
> **前置条件**：Phase 1、2、3 完成。

### 7.1 清理范围

#### 7.1.1 `dstools::labeler` → `dsfw`

`src/framework/ui-core/include/dsfw/IPageActions.h` 和 `IPageLifecycle.h` 中使用了 `namespace dstools::labeler`，但这是框架层接口，应在 `dsfw` 命名空间下。

**修改**：
- `IPageActions.h`：`namespace dstools::labeler` → `namespace dsfw`
- `IPageLifecycle.h`：`namespace dstools::labeler` → `namespace dsfw`
- 所有引用处：`dstools::labeler::IPageActions` → `dsfw::IPageActions`

#### 7.1.2 框架头文件中的 `namespace dstools` using 别名

以下框架头文件末尾有 `namespace dstools { using dsfw::X; }` 向后兼容别名：

| 头文件 | 别名 |
|--------|------|
| `Result.h` | `namespace dstools { using dsfw::Result; }` |
| `ErrorCode.h` | `namespace dstools { using dsfw::ErrorCode; }` |
| `TimePos.h` | `namespace dstools { using dsfw::TimePos; }` |
| `ExecutionProvider.h` | `namespace dstools::infer { using dsfw::infer::ExecutionProvider; }` |
| `AppSettings.h` | `namespace dstools { using dsfw::AppSettings; }` |
| `AsyncTask.h` | `namespace dstools { using dsfw::AsyncTask; }` |
| `ConfigTypesJson.h` | `namespace dstools { using dsfw::...; }` |
| `IDocument.h` | `namespace dstools { using dsfw::IDocument; }` |
| `IFormatAdapter.h` | `namespace dstools { using dsfw::IFormatAdapter; }` |
| `ITaskProcessor.h` | `namespace dstools { using dsfw::ITaskProcessor; }` |
| `PathUtils.h` | `namespace dstools { using dsfw::PathUtils; }` |
| `PipelineContext.h` | `namespace dstools { using dsfw::PipelineContext; }` |
| `PipelineRunner.h` | `namespace dstools { using dsfw::PipelineRunner; }` |
| `PipelineValidators.h` | `namespace dstools { using dsfw::...; }` |
| `ServiceLocator.h` | `namespace dstools { using dsfw::ServiceLocator; }` |
| `TaskProcessorRegistry.h` | `namespace dstools { using dsfw::TaskProcessorRegistry; }` |
| `TaskTypes.h` | `namespace dstools { using dsfw::...; }` |
| `LogNotifier.h` | `namespace dstools { using dsfw::LogNotifier; }` |

**方案**：删除所有 using 别名。应用层代码使用 `dsfw::` 前缀（已有大部分代码使用 `dsfw::`）。

### 7.2 修改文件清单

| 操作 | 文件 |
|------|------|
| 修改 | `IPageActions.h` — `namespace dstools::labeler` → `namespace dsfw` |
| 修改 | `IPageLifecycle.h` — `namespace dstools::labeler` → `namespace dsfw` |
| 修改 | 所有引用 `dstools::labeler::` 的源文件 |
| 删除 | 框架头文件中的 `namespace dstools { using ... }` 块（约 18 个文件） |
| 修改 | 应用层仍使用 `dstools::` 前缀的代码 → 改为 `dsfw::`（如 `dstools::PathUtils` → `dsfw::PathUtils`） |

### 7.3 验收标准

- 框架层头文件（`src/framework/`、`src/types/`）中不再有 `namespace dstools` 定义
- 编译通过
- 所有测试通过

---

## 8. Phase 5：types 移入 framework

> **目标**：将 `src/types/` 目录移入 `src/framework/types/`，逻辑上归入框架层。
> **风险**：低 — 目录移动，CMake target 名不变。
> **前置条件**：Phase 4 完成。

### 8.1 方案

1. 将 `src/types/` 移动到 `src/framework/types/`
2. 更新 `src/CMakeLists.txt` 中的 `add_subdirectory(types)` → `add_subdirectory(framework/types)`
3. 更新 `src/framework/CMakeLists.txt` 中添加 `add_subdirectory(types)`（如果需要）
4. 更新 `dstools-types` target 的别名从 `dstools-types::dstools-types` 改为 `dsfw::types`
5. 更新所有 `find_package(dstools-types)` 为 `find_package(dsfw-types)`（或直接使用 target）

### 8.2 修改文件清单

| 操作 | 文件 |
|------|------|
| 移动 | `src/types/` → `src/framework/types/` |
| 修改 | `src/CMakeLists.txt` — 移除 `add_subdirectory(types)` |
| 修改 | `src/framework/CMakeLists.txt` — 添加 `add_subdirectory(types)` |
| 修改 | `types/CMakeLists.txt` — NAMESPACE `dstools-types::dstools-types` → `dsfw::types` |
| 修改 | 所有 CMakeLists.txt 中的 `dstools-types::dstools-types` → `dsfw::types` |

### 8.3 验收标准

- `src/types/` 目录不存在
- `src/framework/types/` 包含所有原始文件
- 编译通过
- 所有测试通过

---

## 9. Phase 6：CMake NAMESPACE 补全

> **目标**：为所有框架库补充 NAMESPACE 别名，统一 CMake target 命名风格。
> **前置条件**：Phase 1~5 完成。

### 9.1 方案

| Target | 当前别名 | 目标别名 |
|--------|---------|---------|
| `dsfw-audio` | 无 | `dsfw::audio` |
| `infer-common` | 无 | `dsfw::infer` |
| `dsfw-types` | `dsfw::types` | `dsfw::types`（不变） |
| `dsfw-base` | `dsfw::base` | `dsfw::base`（不变） |
| `dsfw-core` | `dsfw::core` | `dsfw::core`（不变） |
| `dsfw-signal` | `dsfw::signal` | `dsfw::signal`（不变） |
| `dsfw-ui-core` | `dsfw::ui-core` | `dsfw::ui-core`（不变） |
| `dsfw-widgets` | `dsfw::widgets` | `dsfw::widgets`（不变） |

同时将 `infer-common` target 重命名为 `dsfw-infer`，与目录名一致。

### 9.2 修改文件清单

| 操作 | 文件 |
|------|------|
| 修改 | `src/framework/audio/CMakeLists.txt` — 添加 `NAMESPACE dsfw::audio` |
| 修改 | `src/framework/infer/CMakeLists.txt` — 重命名 target 为 `dsfw-infer`，添加 `NAMESPACE dsfw::infer` |
| 修改 | 所有引用 `infer-common` 的 CMakeLists.txt → `dsfw::infer` |

### 9.3 验收标准

- 所有框架 target 有 `dsfw::*` 别名
- 编译通过
- 可通过 `target_link_libraries(xxx PRIVATE dsfw::audio)` 正确链接

---

## 10. Phase 7：CMake 目录组织优化

> **目标**：优化 `src/CMakeLists.txt` 中的目录添加顺序，使其与逻辑分层一致。
> **前置条件**：Phase 6 完成。

### 10.1 方案

当前 `src/CMakeLists.txt` 结构：

```cmake
add_subdirectory(types)       # 应移至 framework/
add_subdirectory(framework)
add_subdirectory(domain)
add_subdirectory(ui-core)
add_subdirectory(infer)       # 第三方推理引擎
add_subdirectory(libs)        # 推理适配器
add_subdirectory(apps)
add_subdirectory(tests)
```

优化后：

```cmake
# Framework layer (all dsfw::* targets)
add_subdirectory(framework)   # 包含 types, base, core, signal, infer, audio, ui-core, widgets

# Domain layer
add_subdirectory(domain)

# Application shared layer
add_subdirectory(ui-core)

# Inference engines (third-party)
add_subdirectory(infer)

# Inference adapters
add_subdirectory(libs)

# Applications
add_subdirectory(apps)

# Tests
if (BUILD_TESTS)
    add_subdirectory(tests)
endif()
```

### 10.2 修改文件清单

| 操作 | 文件 |
|------|------|
| 修改 | `src/CMakeLists.txt` — 移除 `add_subdirectory(types)`，添加注释分组 |
| 修改 | `src/framework/CMakeLists.txt` — 确认 `add_subdirectory(types)` 在最前面 |

### 10.3 验收标准

- `src/CMakeLists.txt` 结构清晰，注释分组
- 编译通过

---

## 11. Phase 8：apps/shared 命名空间清理

> **目标**：修复 apps/shared 中的命名空间不规范问题。
> **风险**：极低 — 仅涉及两个命名空间修正。

### 11.1 `chart` 命名空间 → `dstools`

`src/apps/shared/chart-framework/ChartConfigRegistry.h` 使用 `namespace chart`，应改为 `namespace dstools`。

### 11.2 `InferBridge` 命名空间 → `dstools`

`src/libs/infer-bridge/InferBridge.h` 使用 `namespace InferBridge`，应改为 `namespace dstools`。

### 11.3 修改文件清单

| 操作 | 文件 |
|------|------|
| 修改 | `ChartConfigRegistry.h` — `namespace chart` → `namespace dstools` |
| 修改 | 所有引用 `chart::` 的源文件 |
| 修改 | `InferBridge.h` — `namespace InferBridge` → `namespace dstools` |
| 修改 | 所有引用 `InferBridge::` 的源文件 |

### 11.4 验收标准

- `chart` 和 `InferBridge` 命名空间不再存在
- 编译通过

---

## 12. Phase 9：安全加固 — 路径遍历防护

> **目标**：在所有接受用户文件路径的接口处添加路径遍历防护，防止路径逃逸攻击。
> **风险**：低 — 新增防护函数，不影响现有逻辑。
> **对照准则**：ROBUST-01（输入验证）— 所有外部输入必须在边界处验证。

### 12.1 现状

当前项目中，以下位置接受用户提供的文件路径而没有路径遍历校验：

| 位置 | 风险 |
|------|------|
| 格式适配器导入/导出 | 用户可能构造 `../../etc/passwd` 路径访问项目外文件 |
| 配置加载 | 损坏的配置文件可能包含越界路径 |
| 推理引擎模型路径 | 用户可能加载恶意路径下的模型文件 |
| 日志文件路径 | 用户配置的日志路径可能越界 |

### 12.2 方案

1. 在 `dsfw::PathUtils` 中新增 `isPathWithinSandbox()` 函数
2. 在 `dsfw::PathUtils` 中新增 `sanitizePath()` 函数（规范化路径并检查遍历）
3. 在 `dsfw::PathUtils` 中新增 `isPathSafe()` 函数（综合检查：符号链接、遍历、权限）
4. 在所有接受用户路径的入口处调用校验函数

```cpp
// 新增 API（PathUtils.h）
namespace dsfw {

    /// @brief Check if the given path is within the specified root directory.
    /// Prevents path traversal attacks (e.g., "../../etc/passwd").
    /// @param path The path to check.
    /// @param root The sandbox root directory.
    /// @return true if the canonicalized path is within root.
    static bool isPathWithinSandbox(const std::filesystem::path& path,
                                    const std::filesystem::path& root);

    /// @brief Sanitize a user-supplied path by canonicalizing it.
    /// Returns the canonicalized path if it is within the sandbox root,
    /// otherwise returns an error.
    static Result<std::filesystem::path> sanitizePath(const std::filesystem::path& path,
                                                       const std::filesystem::path& root);

    /// @brief Comprehensive path safety check.
    /// Checks for: path traversal, symlink attacks, and absolute paths
    /// when relative paths are expected.
    static Result<std::filesystem::path> validatePath(const std::filesystem::path& path,
                                                       const std::filesystem::path& root,
                                                       bool allowAbsolute = false);
} // namespace dsfw
```

### 12.3 集成点

| 文件 | 集成方式 |
|------|---------|
| `IFormatAdapter` 实现类 | 导入/导出前调用 `sanitizePath()` |
| `DsProject::load()` | 加载前验证项目路径 |
| `DsProject::save()` | 保存前验证目标路径 |
| `AppSettings` 构造函数 | 验证配置目录路径 |
| 各推理引擎适配器 | 模型加载前验证模型路径 |
| `PipelineRunner` | 快照目录路径验证 |

### 12.4 修改文件清单

| 操作 | 文件 |
|------|------|
| 新增 | `PathUtils.h` — 添加 `isPathWithinSandbox()`、`sanitizePath()`、`validatePath()` 声明 |
| 新增 | `PathUtils.cpp` — 添加实现 |
| 新增 | `tests/framework/TestPathSafety.cpp` — 路径安全测试 |
| 修改 | 所有格式适配器实现 — 导入/导出前调用 `sanitizePath()` |
| 修改 | `DsProject.cpp` — 加载/保存前调用 `validatePath()` |

### 12.5 验收标准

- 所有路径遍历测试用例通过
- 编译通过
- 所有现有功能正常
- 安全审查通过

---

## 13. Phase 10：安全加固 — 配置完整性校验

> **目标**：为配置文件加载添加 JSON schema 校验和版本化迁移机制。
> **风险**：中 — 涉及配置文件格式变更，需确保向后兼容。
> **前置条件**：Phase 4 完成。
> **对照准则**：ROBUST-02（向前兼容）— 数据格式应支持版本化迁移。

### 13.1 现状

当前 `DsProject` 的 JSON 配置没有 schema 版本字段，配置损坏时可能产生未定义行为。
`AtomicFileWriter::writeJson()` 已做 JSON 格式校验，但未做语义校验。

### 13.2 方案

1. 在 `DsProject` 的 JSON 格式中新增 `configVersion` 字段（当前版本 = 1）
2. 新增 `DsProject::validate()` 方法，加载后校验 schema 完整性
3. 新增 `ConfigMigration` 类，支持版本到版本的迁移函数
4. `AtomicFileWriter::writeJson()` 在写入前调用 `DsProject::validate()` 进行语义校验

```cpp
// DsProject.h 新增
namespace dstools {

    struct ConfigVersion {
        static constexpr int kCurrent = 1;  ///< Current config schema version.
        static constexpr int kMinSupported = 1;  ///< Minimum supported version.
    };

    class DsProject {
        // ... existing members ...

        /// @brief Validate the project data after loading.
        /// Returns error with details if the data is invalid.
        [[nodiscard]] static Result<void> validate(const nlohmann::json& data);

        /// @brief Migrate old config version to current version.
        /// Returns migrated JSON or error.
        [[nodiscard]] static Result<nlohmann::json> migrate(const nlohmann::json& data, int fromVersion);
    };

} // namespace dstools
```

### 13.3 迁移策略

| 版本 | 变更 | 迁移函数 |
|------|------|---------|
| 1 → 2 | 添加 `configVersion` 字段 | 自动添加 `"configVersion": 2` |
| 2 → 3 | 重命名 `slicerParams` → `slicer.params` | 自动重命名 |
| 后续 | 按需添加 | 注册到 `ConfigMigration` 表 |

### 13.4 修改文件清单

| 操作 | 文件 |
|------|------|
| 修改 | `DsProject.h` — 添加 `validate()`、`migrate()`、`ConfigVersion` |
| 修改 | `DsProject.cpp` — 实现 `validate()`、`migrate()` |
| 新增 | `tests/domain/TestProjectMigration.cpp` — 配置迁移测试 |
| 修改 | `AtomicFileWriter.cpp` — 写入前调用 `validate()` |

### 13.5 验收标准

- 所有配置迁移测试通过
- 旧版本配置可自动迁移到当前版本
- 损坏的配置加载时返回错误（不崩溃）
- 编译通过

---

## 14. Phase 11：可扩展性 — 格式适配器注册机制

> **目标**：标准化格式适配器注册流程，新增格式适配器只需添加新类，不修改核心代码。
> **风险**：低 — 新增注册机制，不影响现有功能。
> **对照准则**：ARCH-09（开闭原则）— 新增格式应只需添加新类，不修改核心代码。

### 14.1 现状

格式适配器通过 `FormatAdapterRegistry` 手动注册，但注册代码分散在各处，新增格式需要在多处添加注册代码。

### 14.2 方案

1. 新增 `IFormatAdapter::registerSelf()` 静态方法，支持自注册模式
2. 新增 `FormatAdapterRegistrar` 宏，在适配器实现文件中自动注册
3. 所有现有格式适配器使用自注册模式

```cpp
// FormatAdapterRegistration.h（新增）
#pragma once

#include <dsfw/IFormatAdapter.h>
#include <dsfw/FormatAdapterRegistry.h>

namespace dsfw {

    /// @brief RAII registrar for format adapters.
    /// Instantiated at file scope to auto-register a format adapter.
    class FormatAdapterRegistrar {
    public:
        FormatAdapterRegistrar(std::unique_ptr<IFormatAdapter> adapter) {
            auto *registry = ServiceLocator::get<FormatAdapterRegistry>();
            if (registry) {
                registry->registerAdapter(std::move(adapter));
            }
        }
    };

} // namespace dsfw

/// @brief Macro to auto-register a format adapter at file scope.
/// Usage: REGISTER_FORMAT_ADAPTER(MyFormatAdapter)
#define REGISTER_FORMAT_ADAPTER(AdapterClass) \
    static dsfw::FormatAdapterRegistrar _registrar_##AdapterClass(std::make_unique<AdapterClass>())
```

### 14.3 修改文件清单

| 操作 | 文件 |
|------|------|
| 新增 | `src/framework/core/include/dsfw/FormatAdapterRegistration.h` — 自注册基础设施 |
| 修改 | 所有格式适配器 .cpp — 添加 `REGISTER_FORMAT_ADAPTER()` 宏 |
| 修改 | 现有手动注册代码 — 移除冗余注册 |

### 14.4 验收标准

- 新增格式适配器只需添加一个 .cpp 文件（含 `REGISTER_FORMAT_ADAPTER` 宏）
- 现有格式适配器功能不变
- 编译通过

---

## 15. Phase 12：可扩展性 — 配置版本化与向前兼容

> **目标**：为所有持久化数据格式添加版本字段，支持自动迁移。
> **风险**：中 — 涉及数据格式变更，需充分测试迁移逻辑。
> **前置条件**：Phase 10 完成。

### 15.1 方案

1. 所有持久化 JSON 格式（DsProject、AppSettings、Pipeline snapshot）添加 `configVersion` 字段
2. 新增 `ConfigMigrator` 通用迁移框架
3. 迁移逻辑集中于 `ConfigMigrator`，各模块注册迁移函数

```cpp
// ConfigMigrator.h（新增）
namespace dstools {

    using MigrationFunc = std::function<nlohmann::json(const nlohmann::json&)>;

    class ConfigMigrator {
    public:
        /// @brief Register a migration from one version to the next.
        static void registerMigration(int fromVersion, MigrationFunc func);

        /// @brief Migrate config data from its current version to the latest.
        static Result<nlohmann::json> migrateToLatest(nlohmann::json data);

        /// @brief Get the current config version.
        static int currentVersion();

    private:
        static std::map<int, MigrationFunc>& migrations();
    };

} // namespace dstools
```

### 15.2 修改文件清单

| 操作 | 文件 |
|------|------|
| 新增 | `src/domain/include/dstools/ConfigMigrator.h` — 迁移框架 |
| 新增 | `src/domain/src/ConfigMigrator.cpp` — 迁移实现 |
| 新增 | `tests/domain/TestConfigMigrator.cpp` — 迁移测试 |
| 修改 | `DsProject.cpp` — 加载时调用 `ConfigMigrator::migrateToLatest()` |
| 修改 | `AppSettings.cpp` — 加载时调用 `ConfigMigrator::migrateToLatest()` |
| 修改 | `PipelineRunner.cpp` — 快照加载时调用迁移 |

### 15.3 验收标准

- 所有迁移测试通过
- 多个版本回退/前进测试通过
- 编译通过

---

## 16. Phase 13：头文件命名统一

> **目标**：统一框架层头文件命名风格，提升代码可读性。
> **风险**：低 — 重命名头文件，更新 include 指令。
> **对照准则**：INFRA-09（命名规范）— 接口用 `I` 前缀，类用 PascalCase。

### 16.1 重命名清单

| 当前文件名 | 目标文件名 | 原因 |
|-----------|-----------|------|
| `curve_tools.h` | 保持不变 | 函数集合保留 snake_case |
| `music_math.h` | 保持不变 | 函数集合保留 snake_case |
| `time_series.h` | 保持不变 | 模板集合保留 snake_case |
| `json_helper.h`（如有） | 保持不变 | 函数集合保留 snake_case |

> **说明**：Phase 13 主要做一致性检查，框架层当前命名基本符合规范。
> 接口类已使用 `I` 前缀，工具类已使用 PascalCase，函数集合已使用 snake_case。
> 本阶段重点是清理不符合规范的命名（如有）并固化命名约定。

### 16.2 修改文件清单

| 操作 | 文件 |
|------|------|
| 检查 | 所有框架层头文件，确认命名符合规范 |
| 修改 | 不符合规范的头文件重命名 |
| 修改 | 所有引用重命名头文件的 include 指令 |
| 新增 | `docs/guides/conventions.md` — 更新头文件命名约定 |

### 16.3 验收标准

- 所有框架层头文件命名符合规范
- 编译通过
- 命名约定文档更新

---

## 17. Phase 14：PIMPL + 前向声明标准化

> **目标**：为复杂度高的类添加 PIMPL 模式，减少头文件编译依赖。
> **风险**：低 — 内部实现重构，不影响外部接口。
> **前置条件**：Phase 4 完成。
> **对照准则**：ARCH-07（PIMPL 隔离）— 复杂类应使用 PIMPL 隐藏实现细节。

### 17.1 现状

| 类 | 已使用 PIMPL | 复杂度 | 是否需要 |
|----|-------------|--------|---------|
| `DsProject` | ✅ | 高 | — |
| `AppSettings` | ✅（AppSettingsData） | 高 | — |
| `AppShell` | ❌ | 高 | 是 |
| `PipelineRunner` | ❌ | 中 | 是 |
| `ServiceLocator` | ❌ | 低 | 否 |
| `PathUtils` | ❌ | 低 | 否（工具类） |
| `AtomicFileWriter` | ❌ | 低 | 否（工具类） |

### 17.2 方案

1. 为 `AppShell` 添加 PIMPL（当前头文件暴露了大量 Qt 依赖）
2. 为 `PipelineRunner` 添加 PIMPL（当前头文件暴露了 `PipelineOptions` 等内部结构体）
3. 审计所有头文件，将不必要的 `#include` 替换为前向声明

### 17.3 前向声明优化

在以下头文件中，将 `#include` 替换为前向声明：

| 头文件 | 当前 | 优化后 |
|--------|------|--------|
| `ITaskProcessor.h` | `#include <dstools/ModelManager.h>` | `class ModelManager;`（前向声明） |
| `IFormatAdapter.h` | `#include <dsfw/TaskTypes.h>` | 保留（需要 LayerData 定义） |
| `ServiceLocator.h` | `#include <dsfw/FormatAdapterRegistry.h>` | 保留（需要完整类型） |

### 17.4 修改文件清单

| 操作 | 文件 |
|------|------|
| 新增 | `AppShellData.h` / `AppShellData.cpp` — PIMPL 实现 |
| 修改 | `AppShell.h` — 移除内部实现细节，使用 unique_ptr<Impl> |
| 新增 | `PipelineRunnerData.h` / `PipelineRunnerData.cpp` — PIMPL 实现 |
| 修改 | `PipelineRunner.h` — 移除 `PipelineOptions` 等内部结构体 |
| 修改 | 所有头文件 — 前向声明优化 |

### 17.5 验收标准

- 头文件编译时间减少（编译依赖减少）
- 接口不变，所有测试通过
- 编译通过

---

## 18. Phase 15：CMake 预设 + 版本号生成

> **目标**：添加 CMakePresets.json 和统一的 version.h 生成机制。
> **风险**：极低 — 新增文件，不影响现有构建。
> **前置条件**：Phase 7 完成。

### 18.1 方案

1. 创建 `CMakePresets.json`（支持 debug/release/release-cli 预设）
2. 创建 `cmake/version.h.in` 模板，通过 `configure_file()` 生成各应用的 version.h
3. 将 vcpkg manifest 移到项目根目录

```json
// CMakePresets.json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "debug",
      "displayName": "Debug",
      "binaryDir": "${sourceDir}/cmake-build-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BUILD_TESTS": "ON"
      }
    },
    {
      "name": "release",
      "displayName": "Release",
      "binaryDir": "${sourceDir}/cmake-build-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "BUILD_TESTS": "OFF"
      }
    },
    {
      "name": "release-cli",
      "displayName": "Release CLI",
      "binaryDir": "${sourceDir}/cmake-build-release-cli",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "BUILD_TESTS": "OFF",
        "BUILD_EXAMPLES": "OFF"
      }
    }
  ]
}
```

```cpp
// cmake/version.h.in
#pragma once
#define @PROJECT_NAME_UPPER@_VERSION_MAJOR @PROJECT_VERSION_MAJOR@
#define @PROJECT_NAME_UPPER@_VERSION_MINOR @PROJECT_VERSION_MINOR@
#define @PROJECT_NAME_UPPER@_VERSION_PATCH @PROJECT_VERSION_PATCH@
#define @PROJECT_NAME_UPPER@_VERSION_STRING "@PROJECT_VERSION@"
```

### 18.2 修改文件清单

| 操作 | 文件 |
|------|------|
| 新增 | `CMakePresets.json` |
| 新增 | `cmake/version.h.in` |
| 移动 | `scripts/vcpkg-manifest/vcpkg.json` → `vcpkg.json` |
| 移动 | `scripts/vcpkg-manifest/vcpkg-configuration.json` → `vcpkg-configuration.json` |
| 修改 | 根 `CMakeLists.txt` — 添加 `configure_file()` 生成 version.h |
| 修改 | 各应用 `CMakeLists.txt` — 使用生成的 version.h |

### 18.3 验收标准

- `cmake --preset release` 可正常使用
- 各应用有正确的版本号
- vcpkg 依赖正常解析

---

## 19. Phase 16：安全加固 — 日志轮转与临时文件清理

> **目标**：添加日志文件大小限制和临时文件自动清理机制。
> **风险**：极低 — 新增守护逻辑，不影响现有功能。

### 19.1 现状

- 日志文件无大小限制，长期运行可能填满磁盘
- PipelineRunner 的快照文件在异常退出后可能残留
- 部分中间文件（推理引擎临时文件）未清理

### 19.2 方案

1. 在 `LogNotifier` 或日志系统中添加日志轮转（保留最近 N 个文件，总大小限制）
2. 在 `PipelineRunner` 中添加 `cleanupOrphanedSnapshots()` 启动时清理
3. 在应用退出时清理临时文件

```cpp
// LogManager.h 新增
namespace dsfw {

    class LogManager {
    public:
        /// @brief Set log rotation policy.
        static void setRotationPolicy(size_t maxFileSize, int keepCount);

        /// @brief Rotate logs if current file exceeds max size.
        static void rotateIfNeeded();

        /// @brief Clean up old log files beyond keep count.
        static void cleanupOldLogs();
    };

} // namespace dsfw
```

### 19.3 修改文件清单

| 操作 | 文件 |
|------|------|
| 新增 | `LogManager.h` / `LogManager.cpp` — 日志轮转 |
| 修改 | `PipelineRunner.cpp` — 添加 `cleanupOrphanedSnapshots()` 调用 |
| 修改 | 应用退出代码 — 添加临时文件清理 |

### 19.4 验收标准

- 日志文件不超过配置的大小限制
- 异常退出后残留的快照文件在下次启动时被清理
- 编译通过

---

## 20. 风险矩阵

| 风险 | 阶段 | 严重度 | 缓解措施 |
|------|------|--------|---------|
| signal 头文件移动后 include 路径失效 | Phase 1 | 低 | include 搜索路径基于 `target_include_directories`，移动后路径不变 |
| `dsfw::music` → `dsfw::signal` 遗漏引用 | Phase 1 | 低 | 编译器会捕获所有未修改的引用 |
| infer 命名空间变更导致推理库编译失败 | Phase 2 | 中 | 先创建向后兼容 shim，逐步迁移 |
| 框架命名空间清理导致大规模编译错误 | Phase 4 | 中 | 逐文件修改，每次编译验证 |
| types 移动后 CMake 配置错误 | Phase 5 | 低 | 仅目录移动，target 名不变 |
| CMake target 重命名影响外部项目 | Phase 6 | 低 | 当前无外部项目依赖 |
| 路径遍历防护误拦截合法路径 | Phase 9 | 中 | 充分测试各种路径格式（相对/绝对/symlink） |
| 配置迁移丢失数据 | Phase 10 | 中 | 每个迁移版本单独测试，保留原始数据备份 |
| 自注册机制与现有手动注册冲突 | Phase 11 | 中 | 先共存，再逐步移除手动注册 |
| PIMPL 重构破坏接口兼容性 | Phase 14 | 低 | 仅修改私有实现，公共接口不变 |

---

## 21. 执行顺序总结

```
Phase 1  (signal 头文件归位)     ──┐
Phase 2  (infer 命名空间统一)    ──┤
Phase 3  (audio 目录扁平化)      ──┤ 可并行
Phase 8  (apps/shared 清理)      ──┤
Phase 13 (头文件命名统一)        ──┘
        │
Phase 4  (框架命名空间清理)      ← 依赖 1, 2, 3
Phase 9  (安全加固 - 路径)       ← 无前置依赖，可与 4 并行
        │
Phase 5  (types 移入 framework)  ← 依赖 4
Phase 10 (安全加固 - 配置)       ← 依赖 4
        │
Phase 6  (CMake NAMESPACE 补全)  ← 依赖 1~5
Phase 11 (可扩展性 - 格式适配器) ← 无前置依赖，可与 5/6 并行
        │
Phase 7  (CMake 目录组织优化)    ← 依赖 6
Phase 14 (PIMPL + 前向声明)      ← 依赖 4
        │
Phase 12 (可扩展性 - 配置版本化) ← 依赖 10
Phase 15 (CMake 预设 + 版本)     ← 依赖 7
Phase 16 (安全加固 - 日志清理)   ← 无前置依赖，可与 15 并行
```

每个阶段完成后提交一次，保持提交粒度可控。

---

## 22. 验收总标准

- [ ] 所有 Phase 1~16 完成
- [ ] 全量编译通过（`cmake --build --preset release`）
- [ ] 所有现有测试通过
- [ ] 框架层无 `namespace dstools` 定义（除向后兼容 shim）
- [ ] 所有框架 target 有 `dsfw::*` 别名
- [ ] 目录结构与目标架构一致
- [ ] 所有用户输入路径经过遍历防护校验
- [ ] 配置文件支持版本化迁移
- [ ] 格式适配器支持自注册
- [ ] 复杂类使用 PIMPL 隔离
- [ ] 头文件命名符合规范
- [ ] `CMakePresets.json` 可用
- [ ] 日志轮转正常工作
- [ ] `human-decisions.md` 速查表逐条复核通过
- [ ]