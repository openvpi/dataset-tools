# dataset-tools 大型重构方案 v6.0

> 版本：6.0.0
> 日期：2026-06-06
> 状态：已完成
> 范围：目录结构、命名空间、CMake 构建系统、安全加固、可扩展性、可读性
> 原则：优先保证稳定性，每阶段自包含可编译，逐阶段单独提交

---

## 0. 核心原则

### 0.1 功能完全不变
所有对外行为保持一致。重构仅改变内部组织方式，不改变可观测行为。

### 0.2 逐阶段编译验证
每个阶段完成后必须通过编译验证，确保无回归。

### 0.3 设计准则优先
所有方案必须通过 [human-decisions.md](../../human-decisions.md) 速查表逐条复核。

### 0.4 最小变更范围
每个提交仅涉及一个阶段的变更，变更范围可控、可回滚。

### 0.5 安全第一
涉及文件 I/O、配置管理、用户输入的所有变更必须通过安全审查。

### 0.6 开闭原则
新增接口/注册机制必须遵循开闭原则（新增类，不修改核心代码）。

---

## 1. 当前状态评估

### 1.1 已完成（v5 Phase 1~8）

| 阶段 | 内容 | 状态 |
|------|------|------|
| Phase 1 | signal 头文件从 core/include 移入 signal/include | ✅ 完成 |
| Phase 2 | infer 命名空间统一为 dsfw::infer，保留向后兼容 shim | ✅ 完成 |
| Phase 3 | audio 目录扁平化，移除 dsfw-audio/ 嵌套 | ✅ 完成 |
| Phase 4 | dstools::labeler 命名空间改为 dsfw（IPageActions/IPageLifecycle） | ✅ 完成 |
| Phase 5 | src/types/ 移入 src/framework/types/ | ✅ 完成 |
| Phase 6 | CMake NAMESPACE 补全（dsfw::audio, dsfw::infer, dsfw::types） | ✅ 完成 |
| Phase 7 | src/CMakeLists.txt 目录组织优化，注释分组 | ✅ 完成 |
| Phase 8 | chart/InferBridge 命名空间统一为 dstools | ✅ 完成 |

### 1.2 当前目标架构状态

```
src/
├── framework/                          # dsfw 通用框架 ✅
│   ├── types/                          # dsfw-types (header-only) ✅
│   │   ├── include/dsfw/               # Result.h, ErrorCode.h, TimePos.h
│   │   └── include/dsfw/infer/         # ExecutionProvider.h
│   ├── base/                           # dsfw-base (Qt-free) ✅
│   ├── core/                           # dsfw-core (Qt Core) ✅
│   ├── signal/                         # dsfw-signal ✅
│   ├── infer/                          # dsfw-infer ✅
│   ├── audio/                          # dsfw-audio ✅
│   ├── ui-core/                        # dsfw-ui-core ✅
│   └── widgets/                        # dsfw-widgets ✅
├── domain/                             # dstools-domain ✅
│   ├── include/dstools/                # 领域模型 + re-exports
│   └── src/
├── ui-core/                            # dstools-ui-core ✅
├── infer/                              # 第三方推理引擎 ✅
│   ├── game-infer/ hubert-infer/ rmvpe-infer/ moe-infer/
│   └── FunAsr/ onnxruntime/
├── libs/                               # 推理适配器 ✅
│   ├── game-infer-lib/ hubert-fa/ lyric-fa/ slicer/
│   ├── rmvpe-pitch/ min-label-lib/ moe-lib/
│   └── textgrid/ infer-bridge/
├── apps/                               # 应用 ✅
│   ├── shared/                         # 11 个共享子模块
│   ├── ds-labeler/ label-suite/ cli/ widget-gallery/
│   └── unified-editor/
└── tests/                              # 测试 ✅
```

---

## 2. 新发现遗留问题

> 以下问题在 v5 计划中未覆盖，通过本次全面代码探索发现。

### 2.1 目录/命名空间遗留问题

| 编号 | 问题 | 位置 | 严重度 |
|------|------|------|--------|
| N1 | JsonHelper.h 命名空间错误 | `src/framework/core/include/dsfw/JsonHelper.h` — 使用 `namespace dstools`，但位于框架层，应是 `namespace dsfw` | 🔴 高 |
| N2 | JsonHelper.h 引用错误 | `#include <dstools/Result.h>` 应改为 `#include <dsfw/Result.h>` | 🔴 高 |
| N3 | 框架头文件 backward compat aliases | 约 30 处 `namespace dstools { using dsfw::X; }` 向后兼容别名未清理 | 🟡 中 |
| N4 | Apps/shared 子模块 CMake target 命名不一致 | `data-sources`, `chart-framework`, `audio-visualizer`, `shared-bridges`, `settings-page` 等无统一前缀 | 🟡 中 |
| N5 | Apps/shared 子模块无 NAMESPACE | 所有 shared 子模块 target 缺少 NAMESPACE 别名 | 🟡 中 |
| N6 | 领域层 CurveTools/F0Curve/MouthCurve 是 re-export | `src/domain/include/dstools/CurveTools.h` 等仅包含 using 声明，本质是代理 | 🟢 低 |
| N7 | 第三方推理引擎使用 `dstools::Result` | game-infer, hubert-infer, rmvpe-infer, moe-infer 中使用 `dstools::Result` 而非 `dsfw::Result` | 🟢 低 |
| N8 | `src/ui-core/` 在顶层，与 `src/framework/ui-core/` 容易混淆 | 两个 ui-core 命名相似，逻辑分组不清晰 | 🟢 低 |

### 2.2 CMake 构建系统遗留问题

| 编号 | 问题 | 位置 | 严重度 |
|------|------|------|--------|
| C1 | 无 CMakePresets.json | 开发者需手动指定编译参数 | 🟡 中 |
| C2 | vcpkg.json 不在项目根目录 | 当前在 `scripts/vcpkg-manifest/vcpkg.json`，IDE 无法自动识别 | 🟡 中 |
| C3 | dstools-types target 名与 CMake package 名不一致 | target 为 `dstools-types`，NAMESPACE 为 `dsfw::types` | 🟢 低 |
| C4 | 无 apps version.h 生成 | 各应用版本号在 CMake 中硬编码，无统一头文件 | 🟢 低 |
| C5 | libs 子模块 NAMESPACE 不完整 | slicer, textgrid, min-label-lib, infer-bridge 等无 NAMESPACE | 🟢 低 |
| C6 | 预编译头使用不一致 | 仅部分模块启用 PCH | 🟢 低 |

### 2.3 安全/健壮性遗留问题

| 编号 | 问题 | 位置 | 严重度 |
|------|------|------|--------|
| S1 | 缺少路径遍历防护 | 所有接受用户文件路径的接口未做路径逃逸检测 | 🔴 高 |
| S2 | 配置加载无完整性校验 | 配置文件加载后未验证 JSON schema 完整性 | 🔴 高 |
| S3 | 缺少输入大小限制 | 格式适配器、推理引擎的输入数据大小无上限 | 🟡 中 |
| S4 | 临时文件未清理 | PipelineRunner 快照文件、中间文件可能残留 | 🟢 低 |
| S5 | 日志文件无大小限制 | 无日志轮转策略，长期运行可能填满磁盘 | 🟢 低 |

### 2.4 可扩展性遗留问题

| 编号 | 问题 | 位置 | 严重度 |
|------|------|------|--------|
| E1 | 格式适配器注册机制分散 | 注册通过 FormatAdapterRegistry 手动管理，新增格式需修改多处 | 🟡 中 |
| E2 | 配置文件 schema 未版本化 | 无 schema 版本字段，前后兼容性依赖手动处理 | 🟡 中 |
| E3 | 缺少配置版本迁移策略 | 配置格式变更后旧版本无法自动迁移 | 🟡 中 |

### 2.5 可读性遗留问题

| 编号 | 问题 | 位置 | 严重度 |
|------|------|------|--------|
| R1 | include 路径约定不一致 | 部分用 `<dsfw/X.h>`，部分用 `<dstools/X.h>`，部分用裸文件名 | 🟡 中 |
| R2 | PIMPL 使用不一致 | AppShell、PipelineRunner 等复杂类未使用 PIMPL | 🟢 低 |
| R3 | 前向声明不充分 | 部分头文件包含不必要的完整头文件 | 🟢 低 |

---

## 3. 目标架构（v6 终态）

### 3.1 目标目录树

```
src/
├── framework/                          # dsfw 通用框架
│   ├── types/                          # dsfw-types (header-only)
│   ├── base/                           # dsfw-base (Qt-free)
│   ├── core/                           # dsfw-core (Qt Core, 含 JsonHelper)
│   ├── signal/                         # dsfw-signal
│   ├── infer/                          # dsfw-infer
│   ├── audio/                          # dsfw-audio
│   ├── ui-core/                        # dsfw-ui-core
│   └── widgets/                        # dsfw-widgets
│
├── domain/                             # dstools-domain
│
├── ui-core/                            # dstools-ui-core (应用层共享 UI)
│
├── engine/                             # 推理引擎 + 适配器（可选：从 infer/ libs/ 合并）
│   ├── engines/                        # 第三方推理引擎
│   │   ├── game-infer/ hubert-infer/
│   │   ├── rmvpe-infer/ moe-infer/
│   │   └── FunAsr/ onnxruntime/
│   └── adapters/                       # 推理适配器
│       ├── game-infer-lib/ hubert-fa/
│       ├── lyric-fa/ slicer/
│       ├── rmvpe-pitch/ min-label-lib/
│       └── moe-lib/ textgrid/ infer-bridge/
│
├── apps/                               # 应用
│   ├── shared/                         # 11 个共享子模块
│   ├── ds-labeler/ label-suite/
│   ├── cli/ widget-gallery/
│   └── unified-editor/
│
└── tests/                              # 测试
```

> **注意**：`src/engine/` 合并是可选的（Phase 10）。如果风险过高，`infer/` 和 `libs/` 保持当前结构不变。

### 3.2 目标命名空间

| 命名空间 | 所属层 | 说明 |
|---------|--------|------|
| `dsfw` | 框架层 | 通用工具类（PathUtils, AppSettings, ServiceLocator, JsonHelper 等） |
| `dsfw::signal` | 框架层 | 信号处理（CurveTools, Slicer, MusicMath, TimeSeries） |
| `dsfw::infer` | 框架层 | 推理接口与公共实现（IInferenceEngine, OnnxEnv, OnnxModelBase） |
| `dsfw::audio` | 框架层 | 音频处理 |
| `dsfw::widgets` | 框架层 | 通用 UI 组件 |
| `dstools` | 应用层 | 领域模型、适配器、页面 |
| `dstools::settings` | 应用层 | 应用设置键 |
| `Game` / `HFA` / `Rmvpe` / `Moe` / `FunAsr` | 推理引擎 | 第三方引擎（保持原样） |

### 3.3 目标 include 路径约定

| 层 | 规范 | 示例 |
|----|------|------|
| 框架层 | `#include <dsfw/Xxx.h>` | `#include <dsfw/PathUtils.h>` |
| 框架子模块 | `#include <dsfw/module/Xxx.h>` | `#include <dsfw/signal/CurveTools.h>` |
| 领域层 | `#include <dstools/Xxx.h>` | `#include <dstools/DsProject.h>` |
| 应用层 | `#include <dstools/Xxx.h>` | `#include <dstools/AppInit.h>` |

### 3.4 目标 CMake Target 命名规范

| 层 | Target 命名 | NAMESPACE 别名 | 示例 |
|----|-----------|---------------|------|
| 框架层 | `dsfw-{module}` | `dsfw::{module}` | `dsfw-audio` → `dsfw::audio` |
| 领域层 | `dstools-{module}` | `dstools::{module}` | `dstools-domain` → `dstools::domain` |
| Apps/shared | `dstools-shared-{module}` | `dstools::shared::{module}` | `dstools-shared-chart` → `dstools::shared::chart` |
| 推理引擎 | `{name}-infer` | `{name}-infer::{name}-infer` | `game-infer` → `game-infer::game-infer` |
| 适配器 | `dstools-{name}` | `dstools::{name}` | `dstools-slicer` → `dstools::slicer` |

---

## 4. 重构阶段

### 4.0 Phase 0：稳定性基线（前置必做）

> **目标**：在开始任何重构前，建立稳定性基线，确保改动可验证、可回滚。
> **严重度**：🔴 高 — 无基线则无法证明重构未引入回归。
> **风险**：极低 — 纯检查操作，无任何代码修改。

#### 4.0.1 任务

1. **运行全部现有测试**，记录通过/失败数作为基线
2. **编译全量**，记录编译警告数作为基线
3. **检查 git 状态**，确认工作区干净（无未提交变更）
4. **创建基线 tag**：`git tag refactor-v6-baseline`（轻量 tag，仅本地）

#### 4.0.2 验收标准

- [ ] 全部测试结果已记录
- [ ] 编译警告数已记录
- [ ] 工作区干净
- [ ] 基线 tag 已创建

---

### 4.1 阶段依赖图

```
Phase 0  (稳定性基线)                ← 前置必做，整个重构的起点

Phase 1  (JsonHelper 命名空间修正)  ──┐
Phase 2  (框架 backward compat 清理) ──┤ 第 1 批：框架层纯净化
Phase 3  (include 路径统一)         ──┘
        │
Phase 4  (Apps/shared CMake 规范化)  ← 依赖 1~3
Phase 4b (libs/ NAMESPACE 补全)      ← 依赖 1~3，可与 4 并行
Phase 5  (安全加固 - 路径遍历防护)   ← 无前置依赖，可与 4 并行
        │
Phase 6  (安全加固 - 配置校验)       ← 依赖 1~3
Phase 7  (可扩展性 - 格式适配器)     ← 无前置依赖，可与 6 并行
        │
Phase 8  (CMake 预设 + 开发体验)     ← 依赖 4, 4b
Phase 8b (dstools-types target 修正) ← 依赖 1~3，可与 8 并行
Phase 9  (PIMPL + 前向声明)          ← 依赖 1~3
        │
Phase 9b (domain re-exports 清理)    ← 依赖 1~3，可与 9 并行
Phase 9c (INFRA-04 ServiceLocator)   ← 依赖 1~3，可与 9 并行
Phase 9d (PCH 一致性)                ← 依赖 1~3，可与 9 并行
        │
Phase 10 (引擎/适配器目录重组)       ← 可选，需评估风险
Phase 11 (安全加固 - 日志/临时文件)  ← 无前置依赖
```

### 4.2 执行顺序

```
第 0 批：Phase 0（稳定性基线，所有后续阶段的前置条件）

第 1 批（可并行）：Phase 1, Phase 2, Phase 3
第 2 批（可并行）：Phase 4, Phase 4b, Phase 5
第 3 批（可并行）：Phase 6, Phase 7
第 4 批（可并行）：Phase 8, Phase 8b, Phase 9, Phase 9b, Phase 9c, Phase 9d
第 5 批：Phase 10（可选）, Phase 11
```

---

## 5. Phase 1：JsonHelper 命名空间修正

> **目标**：修复 `JsonHelper.h` 命名空间错误（`dstools` → `dsfw`），修正 include 引用。
> **严重度**：🔴 高 — 框架层代码使用了应用层命名空间。
> **风险**：中 — 涉及所有引用 `dstools::JsonHelper` 的代码。
> **对照准则**：ARCH-06（依赖倒置）— 框架层代码必须在 `dsfw` 命名空间下。

### 5.1 问题

`src/framework/core/include/dsfw/JsonHelper.h` 当前：
- 使用 `namespace dstools`（应使用 `namespace dsfw`）
- 包含 `#include <dstools/Result.h>`（应为 `#include <dsfw/Result.h>`）

`src/framework/core/src/JsonHelper.cpp` 当前：
- 使用 `namespace dstools`（应使用 `namespace dsfw`）

### 5.2 方案

1. 将 `JsonHelper.h` 中的 `namespace dstools` 改为 `namespace dsfw`
2. 将 `#include <dstools/Result.h>` 改为 `#include <dsfw/Result.h>`
3. 将 `JsonHelper.cpp` 中的 `namespace dstools` 改为 `namespace dsfw`
4. 更新所有引用 `dstools::JsonHelper` 的代码为 `dsfw::JsonHelper`
5. 在 `JsonHelper.h` 末尾添加向后兼容别名（一个版本后移除）：
   ```cpp
   namespace dstools { using dsfw::JsonHelper; }
   ```

### 5.3 影响范围

所有使用 `JsonHelper` 的代码（预计 20+ 文件），包括：
- 领域层适配器（DsFileAdapter, CsvAdapter, AudacityMarkerAdapter 等）
- 应用层（DsProject, DsDocument, AppSettings 等）
- 推理适配器（InferBridge 等）

### 5.4 验收标准

- `JsonHelper.h` 使用 `namespace dsfw`
- `#include <dstools/Result.h>` 改为 `#include <dsfw/Result.h>`
- 所有引用 `dstools::JsonHelper` 改为 `dsfw::JsonHelper`
- 编译通过
- 所有测试通过

---

## 6. Phase 2：框架 backward compat 别名清理

> **目标**：清理框架层头文件中约 30 处 `namespace dstools { using dsfw::X; }` 向后兼容别名。
> **严重度**：🟡 中
> **风险**：中 — 涉及大量文件变更，但每个变更都是机械性的。
> **前置条件**：Phase 1 完成。
> **对照准则**：ARCH-03（接口稳定）— 框架层不应有应用层命名空间定义。

### 6.1 当前状态

以下框架头文件末尾有 `namespace dstools { using dsfw::X; }` 别名：

| 文件 | 别名 |
|------|------|
| `Result.h` | `dstools::Result`, `dstools::Ok`, `dstools::Err` |
| `ErrorCode.h` | `dstools::ErrorCode` |
| `TimePos.h` | `dstools::TimePos` |
| `ExecutionProvider.h` | `dstools::infer::ExecutionProvider` |
| `AppSettings.h` | `dstools::AppSettings` |
| `AsyncTask.h` | `dstools::AsyncTask` |
| `ConfigTypesJson.h` | `dstools::*` |
| `IDocument.h` | `dstools::IDocument` |
| `IFormatAdapter.h` | `dstools::IFormatAdapter` |
| `IModelProvider.h` | `dstools::IModelProvider` |
| `IG2PProvider.h` | `dstools::IG2PProvider` |
| `G2PTypes.h` | `dstools::*` |
| `ITaskProcessor.h` | `dstools::ITaskProcessor` |
| `PathUtils.h` | `dstools::PathUtils` |
| `PipelineContext.h` | `dstools::PipelineContext` |
| `PipelineRunner.h` | `dstools::PipelineRunner` |
| `PipelineValidators.h` | `dstools::*` |
| `ServiceLocator.h` | `dstools::ServiceLocator` |
| `TaskProcessorRegistry.h` | `dstools::TaskProcessorRegistry` |
| `TaskTypes.h` | `dstools::*` |
| `Log.h` | `dstools::Logger` |
| `LogNotifier.h` | `dstools::LogNotifier` |
| `ConfigTypes.h` | `dstools::*` |

### 6.2 方案

1. 删除框架头文件中所有 `namespace dstools { using ... }` 块
2. 更新所有使用 `dstools::` 前缀引用框架类的代码：
   - `dstools::Result` → `dsfw::Result`
   - `dstools::PathUtils` → `dsfw::PathUtils`
   - `dstools::ServiceLocator` → `dsfw::ServiceLocator`
   - 等等
3. 第三方推理引擎（game-infer, hubert-infer, rmvpe-infer, moe-infer）中的 `dstools::Result` 保持不变（它们通过向后兼容 shim 引用）

### 6.3 无法清理的例外

- 第三方推理引擎（`src/infer/`）中的 `dstools::Result`：这些是独立 CMake 项目，不应修改
- 向后兼容 shim 文件（`src/framework/infer/include/dstools/IInferenceEngine.h`）：保留一个版本周期

### 6.4 验收标准

- 框架层头文件（`src/framework/`）中不再有 `namespace dstools` 定义（除 JsonHelper 兼容别名和 infer shim）
- 编译通过
- 所有测试通过

---

## 7. Phase 3：include 路径统一

> **目标**：统一所有 include 路径规范，框架层使用 `<dsfw/...>`，应用层使用 `<dstools/...>`。
> **严重度**：🟡 中
> **风险**：低 — 机械性替换。
> **前置条件**：Phase 1, 2 完成。

### 7.1 方案

1. 修复所有使用 `<dstools/Result.h>` 等框架头文件的 include 为 `<dsfw/Result.h>`
2. 修复所有使用裸文件名（如 `#include "Result.h"`）的 include 为标准路径
3. 统一第三方推理引擎的 include 路径（保持 `<game-infer/Game.h>` 等格式）

### 7.2 修改范围

| 检查项 | 目标 |
|--------|------|
| `#include <dstools/Result.h>` | → `#include <dsfw/Result.h>` |
| `#include <dstools/ErrorCode.h>` | → `#include <dsfw/ErrorCode.h>` |
| `#include <dstools/PathUtils.h>` | → `#include <dsfw/PathUtils.h>` |
| `#include <dstools/TimePos.h>` | → `#include <dsfw/TimePos.h>` |
| 裸文件名 include | → 标准路径 include |

### 7.3 验收标准

- 所有框架层头文件通过 `<dsfw/...>` 引用
- 所有领域层头文件通过 `<dstools/...>` 引用
- 编译通过

---

## 8. Phase 4：Apps/shared CMake 规范化

> **目标**：统一 apps/shared 子模块的 CMake target 命名和 NAMESPACE。
> **严重度**：🟡 中
> **风险**：低 — 仅涉及 CMake target 重命名和别名添加。
> **前置条件**：Phase 1~3 完成。

### 8.1 当前状态

| 当前 target | 目录 | 目标 target | 目标 NAMESPACE |
|------------|------|------------|---------------|
| `data-sources` | data-sources | `dstools-shared-data-sources` | `dstools::shared::data-sources` |
| `chart-framework` | chart-framework | `dstools-shared-chart` | `dstools::shared::chart` |
| `audio-visualizer` | audio-visualizer | `dstools-shared-visualizer` | `dstools::shared::visualizer` |
| `shared-bridges` | bridges | `dstools-shared-bridges` | `dstools::shared::bridges` |
| `settings-page` | settings | `dstools-shared-settings` | `dstools::shared::settings` |
| `log-page` | log-page | `dstools-shared-log` | `dstools::shared::log` |
| `min-label-editor` | min-label-editor | `dstools-shared-min-label` | `dstools::shared::min-label` |
| `mouth-curve-chart` | mouth-curve-chart | `dstools-shared-mouth-curve` | `dstools::shared::mouth-curve` |
| `phoneme-editor` | phoneme-editor | `dstools-shared-phoneme` | `dstools::shared::phoneme` |
| `pitch-editor` | pitch-editor | `dstools-shared-pitch` | `dstools::shared::pitch` |

### 8.2 方案

1. 为每个子模块添加 `NAMESPACE dstools::shared::{name}` 至 CMakeLists.txt
2. 更新所有 `target_link_libraries` 中的目标引用
3. 添加 `EXPORT_SET` 统一导出

### 8.3 验收标准

- 所有 apps/shared 子模块有 NAMESPACE 别名
- target 命名风格统一（`dstools-shared-*`）
- 编译通过

---

## 8b. Phase 4b：libs/ NAMESPACE 补全

> **目标**：为 `src/libs/` 下所有推理适配器 target 补全 NAMESPACE 别名。
> **严重度**：🟡 中
> **风险**：低 — 仅涉及 CMake NAMESPACE 添加。
> **前置条件**：Phase 1~3 完成。
> **对照准则**：INFRA-05（统一路径库）— CMake target 命名与 NAMESPACE 应一致。

### 8b.1 当前状态

当前仅 `textgrid` 有 NAMESPACE（`textgrid::textgrid`），其余 8 个 target 缺失：

| 目标 | 缺失 NAMESPACE |
|------|:---:|
| `game-infer-lib` | ❌ |
| `hubert-fa` | ❌ |
| `lyric-fa` | ❌ |
| `slicer` | ❌ |
| `rmvpe-pitch` | ❌ |
| `min-label-lib` | ❌ |
| `moe-lib` | ❌ |
| `infer-bridge` | ❌ |

### 8b.2 方案

1. 为 8 个缺失 NAMESPACE 的 target 添加 `NAMESPACE dstools::{name}`
2. 同时修正 `textgrid` 的 NAMESPACE 从 `textgrid::textgrid` 改为 `dstools::textgrid`

### 8b.3 验收标准

- 所有 libs/ target 有统一的 `dstools::*` NAMESPACE 别名
- 编译通过

---

## 9. Phase 5：安全加固 — 路径遍历防护

> **目标**：在所有接受用户文件路径的接口处添加路径遍历防护。
> **严重度**：🔴 高
> **风险**：低 — 新增防护函数，不影响现有逻辑。
> **对照准则**：ROBUST-01（输入验证）— 所有外部输入必须在边界处验证。

### 9.1 方案

1. 在 `dsfw::PathUtils` 中新增路径安全函数：
   ```cpp
   namespace dsfw {
       /// 检查路径是否在指定沙箱根目录内
       static bool isPathWithinSandbox(const std::filesystem::path& path,
                                       const std::filesystem::path& root);

       /// 规范化用户提供的路径，检查遍历攻击
       static Result<std::filesystem::path> sanitizePath(const std::filesystem::path& path,
                                                          const std::filesystem::path& root);

       /// 综合路径安全检查
       static Result<std::filesystem::path> validatePath(const std::filesystem::path& path,
                                                          const std::filesystem::path& root,
                                                          bool allowAbsolute = false);
   }
   ```

2. 集成点：
   - 所有 `IFormatAdapter` 实现：导入/导出前调用 `sanitizePath()`
   - `DsProject::load()` / `DsProject::save()`：加载/保存前验证
   - `AppSettings` 构造函数：验证配置目录
   - 推理引擎适配器：模型加载前验证路径

### 9.2 验收标准

- 路径遍历测试用例通过
- 编译通过
- 所有现有功能正常

---

## 10. Phase 6：安全加固 — 配置校验

> **目标**：为配置文件加载添加 JSON schema 校验和版本化机制。
> **严重度**：🔴 高
> **风险**：中 — 涉及配置文件格式变更，需确保向后兼容。
> **前置条件**：Phase 1~3 完成。
> **对照准则**：ROBUST-02（向前兼容）— 数据格式应支持版本化迁移。

### 10.1 方案

1. 在 `DsProject` JSON 格式中新增 `configVersion` 字段（当前版本 = 1）
2. 新增 `DsProject::validate()` 方法，加载后校验 schema 完整性
3. 新增 `ConfigMigration` 基础设施，支持版本间迁移
4. `AtomicFileWriter::writeJson()` 在写入前调用 `validate()`

### 10.2 验收标准

- 配置迁移测试通过
- 旧版本配置可自动迁移到当前版本
- 损坏的配置加载时返回错误（不崩溃）
- 编译通过

---

## 11. Phase 7：可扩展性 — 格式适配器自注册

> **目标**：标准化格式适配器注册流程，新增格式只需添加新类。
> **严重度**：🟡 中
> **风险**：低 — 新增注册机制，不影响现有功能。
> **对照准则**：ARCH-07（开闭原则）— 新增格式应只需添加新类。

### 11.1 方案

1. 新增 `FormatAdapterRegistrar` RAII 注册器
2. 新增 `REGISTER_FORMAT_ADAPTER(AdapterClass)` 宏
3. 所有现有格式适配器迁移到自注册模式

### 11.2 验收标准

- 新增格式适配器只需添加 .cpp 文件
- 现有格式适配器功能不变
- 编译通过

---

## 12. Phase 8：CMake 预设 + 开发体验优化

> **目标**：添加 CMakePresets.json，迁移 vcpkg.json 到根目录，生成 version.h。
> **严重度**：🟡 中
> **风险**：极低 — 新增文件，不影响现有构建。
> **前置条件**：Phase 4 完成。

### 12.1 方案

1. 创建 `CMakePresets.json`（debug/release/release-cli 预设）
2. 移动 `scripts/vcpkg-manifest/vcpkg.json` → 根目录 `vcpkg.json`
3. 创建 `cmake/version.h.in` 模板，为各应用生成 version.h

### 12.2 验收标准

- `cmake --preset release` 可正常使用
- IDE 可自动识别 vcpkg 依赖
- 各应用有正确的版本号

---

## 13. Phase 9：PIMPL + 前向声明优化

> **目标**：为复杂类添加 PIMPL，减少编译依赖。
> **严重度**：🟢 低
> **风险**：低 — 内部实现重构，公共接口不变。
> **前置条件**：Phase 1~3 完成。
> **对照准则**：INFRA-13（PIMPL 隔离）— 复杂类应使用 PIMPL 隐藏实现细节。

### 13.1 方案

1. 为 `AppShell` 添加 PIMPL（当前暴露大量 Qt 依赖）
2. 为 `PipelineRunner` 添加 PIMPL（当前暴露 PipelineOptions 等内部结构体）
3. 审计头文件，将不必要的 `#include` 替换为前向声明

### 13.2 验收标准

- 公共接口不变
- 编译通过
- 所有测试通过

---

## 13b. Phase 8b：dstools-types target 命名修正

> **目标**：修正 `dstools-types` target 与 NAMESPACE 别名不一致的问题。
> **严重度**：🟢 低
> **风险**：极低 — 仅 CMake 别名修正。
> **前置条件**：Phase 1~3 完成。
> **对照准则**：INFRA-05（统一路径库）— target 名与 NAMESPACE 应一致。

### 13b.1 问题

`src/domain/` 中的 `dstools-types` target 使用 `NAMESPACE dstools::types`，但 target 名 `dstools-types` 与 NAMESPACE `dstools::types` 存在歧义（target 名暗示 `dstools` 命名空间，NAMESPACE 暗示 `dstools::types`）。应统一为 `NAMESPACE dstools::types`。

实际检查：`dstools-types` 是 header-only 的 INTERFACE target，其中的类型（`Result.h`, `ErrorCode.h`, `TimePos.h`）已移入 `src/framework/types/`。当前 `dstools-types` target 是 domain 的依赖，但对 domain 自身无实际内容。

### 13b.2 方案

1. 验证 `dstools-types` target 是否仍有实际内容
2. 如有内容，修正 NAMESPACE 为 `dstools::types`
3. 如无内容（仅作为 domain 的依赖别名），考虑移除此 target，直接依赖 `dsfw-types`

### 13b.3 验收标准

- target 名与 NAMESPACE 一致
- 编译通过

---

## 13c. Phase 9b：domain re-exports 清理

> **目标**：清理 `src/domain/include/dstools/CurveTools.h` 等文件中的框架层 re-export。
> **严重度**：🟢 低
> **风险**：低 — 更新引用路径，编译器可捕获遗漏。
> **前置条件**：Phase 1~3 完成。
> **对照准则**：ARCH-03（接口稳定）— domain 层不应 re-export 框架层符号。

### 13c.1 问题

`src/domain/include/dstools/CurveTools.h` 中有 15 行 `using dsfw::signal::*` 声明，将框架层函数 re-export 到 `dstools` 命名空间。同样 `F0Curve.h`、`MouthCurve.h` 等也类似。

### 13c.2 方案

1. 删除 `CurveTools.h` 中所有 `using dsfw::signal::*` 声明
2. 更新所有通过 `dstools::` 命名空间引用这些函数的代码，改为 `dsfw::signal::`
3. 如 `CurveTools.h` 中无剩余内容，考虑移除该文件

### 13c.3 验收标准

- domain 层不再 re-export 框架层符号
- 所有引用已更新为 `dsfw::signal::`
- 编译通过

---

## 13d. Phase 9c：INFRA-04 ServiceLocator → instance() 迁移

> **目标**：将全局服务访问从 `ServiceLocator::get<X>()` 迁移到 `X::instance()` 模式。
> **严重度**：🟡 中
> **风险**：中 — 涉及全局服务访问模式的变更，影响范围较大。
> **前置条件**：Phase 1~3 完成。
> **对照准则**：INFRA-04（简化全局服务访问）— 全局服务优先通过 `instance()` 直接访问。

### 13d.1 方案

1. 为当前通过 ServiceLocator 注册的全局服务添加 `instance()` 静态方法：
   - `ModelManager::instance()`
   - `IG2PProvider::instance()`（或具体实现类）
   - `FormatAdapterRegistry::instance()`
2. 更新所有 `ServiceLocator::get<X>()` 调用为 `X::instance()`
3. ServiceLocator 保留作为测试兼容层（mock 注入）

### 13d.2 验收标准

- 全局服务可通过 `instance()` 直接访问
- 编译通过
- 所有测试通过

---

## 13e. Phase 9d：PCH 使用一致性

> **目标**：统一预编译头的使用策略，消除不一致。
> **严重度**：🟢 低
> **风险**：极低 — 仅影响编译速度，不影响功能。
> **前置条件**：Phase 1~3 完成。

### 13e.1 方案

1. 审计当前 PCH 配置：哪些模块启用了 PCH，哪些未启用
2. 为所有 framework 模块统一启用 PCH（`<QtCore>`, `<dsfw/Result.h>`, `<dsfw/ErrorCode.h>` 等高频头文件）
3. 为所有 apps 模块统一启用 PCH

### 13e.2 验收标准

- PCH 策略一致
- 编译通过

---

## 14. Phase 10：引擎/适配器目录重组（可选）✅ 已完成

> **目标**：将 `src/infer/` 和 `src/libs/` 合并为 `src/engine/engines/` 和 `src/engine/adapters/`。
> **严重度**：🟢 低
> **风险**：中 — 涉及大量目录移动和 CMake 配置更新。
> **完成时间**：2026-06-07
> **提交**：`refactor(phase10): reorganize infer/libs into engine/engines/adapters directory`

### 14.1 方案

1. 创建 `src/engine/engines/` 和 `src/engine/adapters/` 目录
2. 将 `src/infer/game-infer/` 等移至 `src/engine/engines/`
3. 将 `src/libs/game-infer-lib/` 等移至 `src/engine/adapters/`
4. 更新 CMake 配置

### 14.2 验收标准

- 目录结构清晰分组
- 编译通过
- 所有推理功能正常

---

## 15. Phase 11：安全加固 — 日志轮转与临时文件清理 ✅ 已完成

> **目标**：添加日志轮转和临时文件自动清理。
> **严重度**：🟢 低
> **风险**：极低 — 新增守护逻辑，不影响现有功能。
> **完成时间**：2026-06-07
> **提交**：`refactor(phase11): add log rotation, orphaned snapshot cleanup, and temp file cleanup on startup`

### 15.1 方案

1. 在 `LogNotifier` 中添加日志轮转（保留最近 N 个文件，总大小限制）
2. 在 `PipelineRunner` 中添加 `cleanupOrphanedSnapshots()` 启动时清理
3. 在应用退出时清理临时文件

### 15.2 验收标准

- 日志文件不超过配置的大小限制
- 异常退出后残留文件在下次启动时被清理
- 编译通过

---

## 16. 回滚策略

> 每个阶段独立提交，出现问题时可精确回滚到上一阶段，无需整体回退。

### 16.1 回滚粒度

| 回滚级别 | 操作 | 适用场景 |
|---------|------|---------|
| 单阶段回滚 | `git revert <commit>` | 某阶段编译失败或测试失败 |
| 整批回滚 | `git reset --hard refactor-v6-baseline` | 严重问题，全部回退到基线 |

### 16.2 每个阶段回滚前检查

1. 回滚前编译验证当前阶段的改动是否真的导致问题
2. 确认问题不是由未提交的本地修改导致
3. 回滚后重新编译 + 运行测试，确认恢复

### 16.3 不可回滚的阶段

以下阶段仅新增文件/函数，不修改现有代码，回滚即删除新增内容：

- Phase 0（基线 tag）：删除 tag 即可
- Phase 5（路径遍历防护）：新增 `PathUtils` 函数，不破坏现有接口
- Phase 7（格式适配器自注册）：新增注册机制，与手动注册共存
- Phase 8（CMake 预设）：新增 `CMakePresets.json`，不影响现有构建

---

## 17. 风险矩阵

| 风险 | 阶段 | 严重度 | 缓解措施 |
|------|------|--------|---------|
| JsonHelper 命名空间变更导致大规模编译错误 | Phase 1 | 中 | 先添加向后兼容别名，逐步迁移 |
| 框架别名清理遗漏引用 | Phase 2 | 中 | 编译器会捕获所有未修改的引用 |
| include 路径替换遗漏 | Phase 3 | 低 | 编译器会报告缺失头文件 |
| Apps/shared target 重命名影响链接 | Phase 4 | 低 | 仅 CMake 变更，target 内容不变 |
| libs/ NAMESPACE 添加后链接失败 | Phase 4b | 低 | NAMESPACE 是纯别名，不影响 target 内容 |
| 路径遍历防护误拦截合法路径 | Phase 5 | 低 | 充分测试各种路径格式 |
| 配置迁移丢失数据 | Phase 6 | 中 | 每个迁移版本单独测试，保留原始数据备份 |
| 自注册机制与手动注册冲突 | Phase 7 | 中 | 先共存，再逐步移除手动注册 |
| vcpkg.json 移动影响 CI | Phase 8 | 低 | 更新 CI 脚本中的 manifest 路径 |
| dstools-types target 移除后链接失败 | Phase 8b | 低 | 编译器会报告缺失依赖 |
| PIMPL 重构破坏接口兼容性 | Phase 9 | 低 | 仅修改私有实现，公共接口不变 |
| domain re-export 清理后引用丢失 | Phase 9b | 低 | 编译器会捕获所有缺失引用 |
| ServiceLocator → instance() 迁移遗漏 | Phase 9c | 中 | 先并行运行，再逐步移除 ServiceLocator 调用 |
| PCH 配置错误导致编译失败 | Phase 9d | 极低 | PCH 仅优化编译速度，可随时禁用 |
| 引擎目录重组导致路径错误 | Phase 10 | 中 | 充分测试，必要时回退 |
| 日志轮转策略过激丢失日志 | Phase 11 | 低 | 保留足够大的日志文件数和总大小限制 |

---

## 18. 验收总标准

- [x] Phase 0 基线已建立（测试结果、编译警告数已记录）
- [x] Phase 10 已完成（引擎/适配器目录重组）
- [x] 所有 Phase 1~11 + 4b/8b/9b/9c/9d 完成
- [x] 编译通过（CLion MCP 增量编译验证通过）
- [x] 所有现有测试通过，无新增失败
- [x] 编译警告数不增加（对比 Phase 0 基线）
- [x] 框架层无 `namespace dstools` 定义（除向后兼容 shim 和 JsonHelper 兼容别名）
- [x] 所有框架 target 有 `dsfw::*` 别名
- [x] 所有 apps/shared 子模块有 `dstools::shared::*` 别名
- [x] 所有 libs/ target 有 `dstools::*` 别名
- [x] 所有用户输入路径经过遍历防护校验
- [x] 配置文件支持版本化迁移
- [x] 格式适配器支持自注册
- [x] 复杂类使用 PIMPL 隔离
- [x] `CMakePresets.json` 可用
- [x] `vcpkg.json` 在项目根目录
- [x] 日志轮转正常工作
- [x] domain 层不再 re-export 框架层符号
- [x] 全局服务可通过 `instance()` 直接访问
- [x] PCH 策略一致
- [x] `human-decisions.md` 速查表逐条复核通过
- [x] include 路径规范统一（框架 `<dsfw/...>`，应用 `<dstools/...>`）
- [x] 每个阶段独立提交，可单独回滚