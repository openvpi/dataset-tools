# dataset-tools 精简重构方案

> 版本：4.1.0
> 日期：2026-06-08
> 状态：方案阶段
> 取代：v4.0.0（已废除，因多处错误：错误地将 infer-bridge/unified-editor 判定为可合并薄模块、错误地认为 FileDialogHelper/PathSelector 与 FilePathSelector 功能重叠、错误地认为 JsonHelper 归属不当、错误地将 data-sources 目录重组作为必要项）

---

## 0. 前置声明

### 0.1 方案目标

1. 保证当前全部功能和行为完全不变，只做适当精简
2. 保证实际使用中工程数据的稳定性、安全性
3. 符合 [human-decisions.md](../../human-decisions.md) 全部设计准则
4. 抛弃一切技术债和架构债，去掉历史包袱
5. 统一底层接口：跨平台路径和编码问题由固定模块负责解析与预处理
6. 补充 C++20/Qt6 优秀项目的工程准则
7. 保守的性能优化：不改变原始逻辑，只做工程上等价优化

### 0.2 方案约束

- 功能零退化：所有现有功能保持完全一致，行为不变
- 数据零风险：所有文件 I/O 操作保持原子性，数据完整性校验不变
- 编译零新增警告：重构后编译通过，无新增 warning
- 不引入任何新的外部依赖
- 变更粒度：每个任务独立提交，不推送

### 0.3 取舍原则

**纳入标准**：满足以下全部条件才纳入方案：
1. 问题真实存在且已通过代码审查逐文件验证
2. 修复后能消除实际风险（编译风险、维护风险、行为不一致风险）
3. 变更风险可控（编译器可验证，或机械替换）
4. 符合 ARCH-07（开闭原则），不修改稳定核心逻辑

**剔除标准**：满足以下任一条件即剔除：
1. 纯组织性调整（如目录重命名）——无实际危害，收益不抵 churn
2. 单类模块——ARCH-04 要求 >60% 相同才合并，单类模块不触发
3. 已有合规实现（如信号槽已全部使用新式语法）
4. 变更范围过大、风险不可控
5. 仅为美观调整（如 `using namespace` 去重不影响编译和运行）

---

## 1. 补充准则（SUP-11 ~ SUP-18）

以下准则基于 C++20/Qt6 优秀项目（LLVM、KDE Frameworks、Qt 官方最佳实践）的工程经验补充：

| 编号 | 准则 | 说明 |
|------|------|------|
| SUP-11 | 编译期检查优先 | 能用 `static_assert` / `consteval` / `if constexpr` 验证的约束不在运行时检查 |
| SUP-12 | 头文件最小化 | 头文件只包含必要的 `#include`，优先使用前向声明；IWYU 原则 |
| SUP-13 | 不可变对象优先 | 能用 `const` 标记的就用；配置对象创建后不可变 |
| SUP-14 | 文件对话框统一入口 | 应用层文件选择通过 `dsfw::widgets::FilePathSelector` 或 `dsfw::widgets::PathSelector`，禁止直接调用 `QFileDialog` 静态方法 |
| SUP-15 | 资源文件集中管理 | 每个模块的 `.qrc` 文件统一放在模块根目录，图标资源使用统一的命名前缀 |
| SUP-16 | CMake target 依赖显式化 | 每个 `target_link_libraries` 必须显式列出所有直接依赖，不依赖传递依赖的隐式可用性 |
| SUP-17 | 模块头文件目录与命名空间一致 | `include/dsfw/audio/` 对应 `dsfw::audio` 命名空间；`include/dstools/` 对应 `dstools` 命名空间，禁止交叉放置 |
| SUP-18 | 测试文件与源文件同目录结构 | `src/tests/` 的目录结构镜像 `src/` 的模块结构，测试文件名以 `Test` 前缀与源文件对应 |

---

## 2. 现状分析

### 2.1 逐文件核实的问题清单

对每个问题均通过读取实际文件内容、检查 CMakeLists.txt 依赖关系、搜索引用链进行核实。

#### P1: `dsfw-base` 幽灵模块（核验通过）

| 项目 | 核实结果 |
|------|----------|
| 源文件 | `src/framework/base/include/dsfw/` 为空，`src/framework/base/src/` 为空 |
| CMakeLists.txt | 创建 `dsfw::base` 静态库，PUBLIC 依赖 `dsfw::types` 和 `nlohmann_json`，无任何源文件 |
| 被引用链 | `dsfw-core` (PUBLIC) → `dsfw-base`；`tests/domain` (PRIVATE, 2 处) → `dsfw-base` |
| 实际作用 | 仅作为依赖传递层，将 `nlohmann_json` 和 `dsfw::types` 传播给消费者 |
| 架构文档 | `overview.md` 声称 `dsfw-base` 包含 `JsonHelper`，但 `JsonHelper.h` 实际在 `src/framework/core/include/dsfw/JsonHelper.h`，属于 `dsfw-core` |

**结论**：`dsfw-base` 是空模块，无源码，唯一作用是传递依赖。移除后 `dsfw-core` 可直接声明 `nlohmann_json` 和 `dsfw::types` 依赖（`dsfw-core` CMakeLists.txt 第 26 行已同时声明两者）。

#### P2: 空 `include/dstools/` 目录（核验通过）

| 目录 | 核实结果 |
|------|----------|
| `src/framework/core/include/dstools/` | 空，无任何文件 |
| `src/framework/types/include/dstools/` | 空，无任何文件 |
| `src/framework/infer/include/dstools/` | 空，无任何文件 |

**结论**：三个空目录违反 SUP-17（模块头文件目录与命名空间一致）。`dstools` 命名空间的实际头文件在 `src/domain/include/dstools/` 和 `src/ui-core/include/dstools/`。

#### P3: 架构文档与实际构建系统不一致（核验通过）

| 文档声称 | 实际状态 |
|----------|----------|
| `dsfw-base` 为 Qt-free 静态库，含 `JsonHelper` | `dsfw-base` 无源码，`JsonHelper` 在 `dsfw-core` |
| `dstools-widgets` 为 INTERFACE header-only 层 | 构建系统中不存在此 target |
| `dsfw-core` 依赖 `dsfw-base` | 仅依赖传递，`dsfw-core` 已直接声明 `nlohmann_json` |

**结论**：架构文档 `overview.md` 需要更新以反映实际构建系统。

#### 以下问题经核实不纳入

| 编号 | 原始主张 | 核实结论 | 不纳入原因 |
|------|----------|----------|------------|
| - | 合并 `audio/playback` 子模块 | `playback` 子模块封装 `AudioPlayerAdapter`（PIMPL 模式），是 `IAudioPlayerAdapter` 的独立实现层，遵循 ARCH-01（模块职责单一）。仅被 `dsfw-widgets` 的 `PlayWidget` 引用 | 职责独立，合并会破坏关注点分离 |
| - | 合并 `infer-bridge` 模块 | `infer-bridge` 聚合 5 个推理引擎（hubert-infer, rmvpe-infer, game-infer, lyricfa-lib, dsfw-infer），作为 `EngineTraits` 特化和 `ModelProviderInit` 集中注册点。被 4 个 target 引用 | 职责明确，合并会引入循环依赖风险 |
| - | 合并 `unified-editor` 模块 | header-only 模块，提供 `AppShellConfig` 工具函数，被 `label-suite` 和 `ds-labeler` 引用 | 头文件仅含 `inline`/`template` 函数，无编译开销，遵循 ARCH-04 |
| - | 整理 `data-sources/` 目录为子目录结构 | 47 个文件已按命名约定清晰分组（Page/Service/Model/Factory/Dialog），编译为单个 CMake target | 纯组织性调整，收益不抵 47 个文件的 include 路径变更风险 |
| - | `JsonHelper` 归属不当 | `JsonHelper` 在 `dsfw-core` 中，被 14 个文件引用（含 `dsfw-core`、`domain`、`engine` 层）。`dsfw-core` 是 Qt-free 工具集的合理位置 | 归属正确，无需变更 |
| - | `FileDialogHelper` 与 `FilePathSelector` 功能重叠 | `FileDialogHelper` 是 header-only 静态工具类（直接封装 `QFileDialog::get*`），`FilePathSelector` 是有状态 QObject 封装（含 Config 和 signals）。两者层次不同，`FileDialogHelper` 是 `FilePathSelector` 和 `PathSelector` 的底层实现细节 | 层次分明，无重叠 |
| - | `PathSelector` 与 `FilePathSelector` 冲突 | `PathSelector` 是 QWidget（含 browse button + drag-drop），`FilePathSelector` 是 QObject（无 UI）。两者用于不同场景：Settings 面板用 `PathSelector`（内联路径选择），对话框/页面用 `FilePathSelector`（抽象文件选择） | 用途不同，无冲突 |

### 2.2 已核验合规项（无需重构）

| 项目 | 状态 |
|------|------|
| `path().string()` 使用 | 仅在 `PathUtils.cpp` 自身实现中使用，0 处业务代码违规 |
| `processEvents()` 使用 | 全项目 0 处 |
| 信号槽语法 | 全部使用新式语法（成员函数指针） |
| 头文件 `using namespace dsfw` | 33 个头文件已清理（v3.0 完成） |
| 音频模块设计文档 | 已更新状态（v3.0 完成） |
| 裸 `new`/`delete` | 仅在第三方引擎代码中存在，项目自有代码已合规 |
| `ServiceLocator::get()` | 仅在 `ServiceLocator` 自身和 `main.cpp` 中使用 |
| `vector::reserve()` 优化 | 全项目 30+ 处已使用 `reserve()` 预分配，无需额外优化 |

### 2.3 性能优化：逐文件核验

对 `QString`/`std::string` 转换、容器操作等热点路径进行逐文件检查：

| 位置 | 模式 | 是否优化 | 说明 |
|------|------|----------|------|
| `ChartConfigRegistry::loadDefaults()` | `chartId.toStdString()` 在同一循环中调用 2 次 | 是 | 缓存为局部变量 `std::string key = chartId.toStdString()` |
| `ChartConfigRegistry::saveConfig()` | `it.key().toStdString()` 在循环体内每次迭代调用 | 是 | 缓存为局部变量，减少重复编码转换 |
| `AudioPipeline::decodeAndResample()` | `chunks.push_back(std::move(...))` 无 `reserve()` | 否 | 分块数量取决于文件大小，无法预知 |
| 其他 `toStdString()` 调用 | 均为单次调用或日志输出 | 否 | 单次调用无优化空间 |

---

## 3. 重构方案

### 3.1 阶段一：消除 `dsfw-base` 幽灵模块（P1）

**目标**：移除空的 `dsfw-base` 模块，消除构建图中的无效节点。

**变更范围**：

1. 删除 `src/framework/base/` 目录（含 CMakeLists.txt、空 include/、空 src/）
2. 从 `src/framework/CMakeLists.txt` 移除 `add_subdirectory(base)` 行
3. 修改 `src/framework/core/CMakeLists.txt`：将 `PUBLIC dsfw-base` 替换为 `PUBLIC dsfw::types nlohmann_json::nlohmann_json`
   - `dsfw-base` 的唯一 PUBLIC 依赖就是 `dsfw::types` 和 `nlohmann_json::nlohmann_json`，直接声明完全等价
4. 修改 `src/tests/domain/CMakeLists.txt`：将 2 处 `dsfw-base` 替换为 `dsfw-core`

**风险**：极低。`dsfw-base` 无源码，移除后依赖关系由 `dsfw-core` 完整继承。编译器可验证。

**验证**：编译通过，所有测试通过。

---

### 3.2 阶段二：清理空 `include/dstools/` 目录（P2）

**目标**：移除违反 SUP-17 的空目录。

**变更范围**：

1. 删除 `src/framework/core/include/dstools/`（空目录）
2. 删除 `src/framework/types/include/dstools/`（空目录）
3. 删除 `src/framework/infer/include/dstools/`（空目录）

**风险**：零。空目录不参与编译，删除不影响任何构建行为。

**验证**：目录列表确认已删除。

---

### 3.3 阶段三：保守性能优化（仅 2 处、等价替换）

**目标**：消除 `ChartConfigRegistry` 中重复的 `QString::toStdString()` 编码转换。

**变更范围**：

1. `ChartConfigRegistry::loadDefaults()`（`src/apps/shared/chart-framework/ChartConfigRegistry.cpp` 第 82-87 行）：
   - 缓存 `chartId.toStdString()` 为局部变量 `std::string chartIdStr`
   - 缓存 `param.key.toStdString()` 为局部变量 `std::string key`
   - 逻辑完全等价，仅减少重复编码转换

2. `ChartConfigRegistry::saveConfig()`（`src/apps/shared/chart-framework/ChartConfigRegistry.cpp` 第 109-111 行）：
   - 在第 108 行循环内缓存 `it.key().toStdString()` 为局部变量
   - 逻辑完全等价

**风险**：零。纯等价替换，不改变任何逻辑。

**验证**：编译通过，`ChartConfigRegistry` 行为不变。

---

### 3.4 阶段四：更新架构文档（P3）

**目标**：使 `overview.md` 与实际构建系统一致。

**变更范围**：

1. 更新 `docs/developer/architecture/overview.md`：
   - 移除 `dsfw-base` 层（Layer 0.5）
   - 将 `JsonHelper` 从 `dsfw-base` 层移至 `dsfw-core` 层
   - 移除 `dstools-widgets INTERFACE` 层（构建系统中不存在）
   - 更新模块依赖图：`dsfw-core` 直接依赖 `dsfw::types` + `nlohmann_json`
   - 更新模块层次结构表

2. 更新 `docs/human-decisions.md`：
   - 更新最后更新日期和变更说明
   - 确认 SUP-11~SUP-18 补充准则已添加

**风险**：零。纯文档更新。

---

## 4. 实施计划

### 4.1 阶段划分

| 阶段 | 内容 | 优先级 | 风险 | 影响范围 |
|------|------|--------|------|----------|
| 一 | 消除 `dsfw-base` 幽灵模块 | 高 | 极低 | 1 个目录删除 + 3 个 CMakeLists.txt 修改 |
| 二 | 清理空 `include/dstools/` 目录 | 高 | 零 | 3 个空目录删除 |
| 三 | 保守性能优化（ChartConfigRegistry） | 中 | 零 | 1 个文件 2 处等价替换 |
| 四 | 更新架构文档 | 低 | 零 | 2 个文档文件 |

### 4.2 执行顺序

1. 阶段一：消除幽灵模块（优先执行，风险最低）
2. 阶段二：清理空目录（独立，可并行）
3. 阶段三：保守性能优化（独立，可并行）
4. 阶段四：更新架构文档（依赖前三个阶段完成）

### 4.3 每个阶段的执行流程

1. 执行代码变更
2. 编译通过（CLion MCP 增量编译）
3. 运行单元测试
4. 提交（不推送）

---

## 5. 不做的事及原因

| 内容 | 剔除原因 |
|------|----------|
| 合并 `audio/playback` 子模块 | 职责独立，`IAudioPlayerAdapter` 接口 + `AudioPlayerAdapter` PIMPL 实现是合理的关注点分离，遵循 ARCH-01 |
| 合并 `infer-bridge` 模块 | 聚合 5 个推理引擎的桥接层，合并会引入循环依赖风险 |
| 合并 `unified-editor` 模块 | header-only 工具模块，无编译开销，遵循 ARCH-04 |
| 整理 `data-sources/` 目录结构 | 纯组织性调整，47 个文件的 include 路径变更风险大于收益 |
| 修正 `JsonHelper` 归属 | `dsfw-core` 是 Qt-free 工具集的正确位置，归属正确 |
| 合并 `FileDialogHelper` 与 `FilePathSelector` | 不同层次：`FileDialogHelper` 是底层静态工具，`FilePathSelector` 是上层有状态封装 |
| 替换 `PathSelector` 为 `FilePathSelector` | 不同用途：`PathSelector` 是 QWidget（内联路径选择），`FilePathSelector` 是 QObject（抽象文件选择） |
| 删除重复的 `using namespace dsfw;`（25 个文件） | 纯格式问题，不影响编译和运行。已纳入编码规范检查，待后续 clang-format 统一处理 |
| PIMPL 隔离第三方头文件 | 涉及 `nlohmann/json`、`onnxruntime` 等头文件暴露，风险不可控 |
| 接口版本化全面推行 | 存量接口无多实现需求，暂不强制 |
| 从 Sqlite 迁移音频文件注册表 | 涉及数据格式变更，不在本次精简重构范围内 |
| 统一 `AudioEditorWidgetBase` 和 `EditorContainerBase` | 两个基类有不同职责（编辑器 vs 容器），遵循 ARCH-05 组合优于继承 |

---

## 6. 附录：变更文件清单

### 阶段一

```
删除：src/framework/base/                                  （整个目录，含 CMakeLists.txt + 空 include/ + 空 src/）
修改：src/framework/CMakeLists.txt                          （移除 add_subdirectory(base)）
修改：src/framework/core/CMakeLists.txt                     （dsfw-base → dsfw::types + nlohmann_json::nlohmann_json）
修改：src/tests/domain/CMakeLists.txt                       （2 处 dsfw-base → dsfw-core）
```

### 阶段二

```
删除：src/framework/core/include/dstools/                   （空目录）
删除：src/framework/types/include/dstools/                  （空目录）
删除：src/framework/infer/include/dstools/                   （空目录）
```

### 阶段三

```
修改：src/apps/shared/chart-framework/ChartConfigRegistry.cpp （2 处 toStdString() 缓存优化）
```

### 阶段四

```
修改：docs/developer/architecture/overview.md
修改：docs/human-decisions.md
```