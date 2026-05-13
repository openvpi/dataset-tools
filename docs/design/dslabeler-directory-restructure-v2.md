# DsLabeler 目录重构方案（v2 — 实际代码分析版）

> 2026-05-13 — 基于全量代码交叉引用分析，聚焦消除重复和精简模块
>
> 设计准则见 `human-decisions.md`

---

## 1 现状全量分析

### 1.1 当前文件清单

```
src/apps/ds-labeler/                    (15 个源文件平铺在同一目录)
├── CMakeLists.txt
├── main.cpp                            287行 — 入口 + 注册 + 生命周期 + 一致性校验
├── DsLabelerKeys.h                      32行 — 快捷键/配置键定义
├── DsSlicerPage.h / .cpp               555行 — 切片页（工程集成）
├── ExportPage.h / .cpp                1288行 — 导出页（UI+校验+导出+引擎）
├── LayerDependencyGraph.h / .cpp       113行 — 层依赖DAG
├── NewProjectDialog.h / .cpp           173行 — 新建工程对话框
├── ProjectDataSource.h / .cpp          310行 — 工程数据源
├── WelcomePage.h / .cpp                266行 — 欢迎页
└── translations/
```

### 1.2 交叉引用全景（实际 grep 结果）

```
┌──────────────────────────────────────────────────────────────────┐
│ 文件               │ 被谁引用 ( #include )                       │
├────────────────────┼─────────────────────────────────────────────┤
│ DsLabelerKeys.h    │ ❌ 零引用 — 整个代码库无任何文件包含它      │
│ LayerDependencyGraph│ ❌ 零引用 — 仅自己的 .cpp 包含 .h          │
│ WelcomePage.h      │ main.cpp                                    │
│ NewProjectDialog.h │ WelcomePage.cpp (仅此一处)                  │
│ DsSlicerPage.h     │ main.cpp                                    │
│ ExportPage.h       │ main.cpp                                    │
│ ProjectDataSource.h│ main.cpp, DsSlicerPage.cpp, ExportPage.cpp │
└────────────────────┴─────────────────────────────────────────────┘
```

**关键发现：**
- `DsLabelerKeys.h` — **死代码**，32 行零引用
- `LayerDependencyGraph` — **死代码**，113 行零引用（设计文档 ADR-57 提及但从未接入管线）

### 1.3 真实代码重复分析

#### 重复 1：ExportPage 推理逻辑两份拷贝

`ExportPage.cpp` 中存在 **~150 行完全相同的推理逻辑**，出现在两个位置：

| 位置 | 行号 | 场景 |
|------|------|------|
| `autoCompleteSlice()` | L394-L592 | 单切片补全，**主线程同步调用**（违反 P-03） |
| `onExport()` lambda | L634-L904 | 批量补全，`QtConcurrent::run` 后台线程 |

两处共享的核心逻辑（逐行对比一致）：
```
1. 扫描 doc.layers / doc.curves 判断 hasGrapheme/hasPhoneme/hasPhNum/hasPitch/hasMidi
2. 缺少 phoneme 层 → m_hfa->recognize() → 构建 phonemeLayer → 替换或追加到 doc.layers
3. 缺少 ph_num 层 → m_phNumCalc->calculate() → 构建 phNumLayer → 追加
4. 缺少 pitch  层 → m_rmvpe->get_f0() → 构建 pitchCurve
5. 缺少 midi   层 → m_game->getNotes() → 构建 midiLayer
```

> **差异仅在**：批量版多了 `QMetaObject::invokeMethod` 进度回调、`try-catch` 包裹、存活令牌检查。

#### 重复 2：main.cpp 一致性校验逻辑内联

`main.cpp` L155-L270（~115 行）的 `currentPageChanged` lambda 包含了完整的切片一致性校验逻辑。这段代码与页面注册/生命周期管理混在一起，使 main.cpp 臃肿。

#### 无重复可合并的模块

| 对比 | 结论 |
|------|------|
| NewProjectDialog ↔ WelcomePage | 无代码重复，仅使用关系；合并不会消除任何重复，反而让 WelcomePage 增至 ~336 行 |
| DsSlicerPage ↔ SlicerPage | DsSlicerPage 通过**继承**复用 SlicerPage，符合 P-12/ADR-127（已完成） |
| 各 Page 间 | 各自独立职责，通过 ProjectDataSource 共享数据，无重复 |

---

## 2 重构方案

### 2.1 核心原则

| 原则 | 说明 |
|------|------|
| **消除死代码** | 删除零引用文件 |
| **消除重复代码** | 合并 ExportPage 两份推理逻辑 |
| **精简 main.cpp** | 一致性校验改为独立函数 |
| **按职责分层** | core/（数据层）vs ui/（页面层） |
| **不新增文件模块** | 通过内部重构而非拆分类来解决问题 |
| **目录不膨胀** | 最多 2 层，每目录 ≤ 8 个文件 |

### 2.2 目标目录结构

```
src/apps/ds-labeler/                     (共 12 个源文件)
├── CMakeLists.txt
├── main.cpp                             ~170行 (精简)
├── translations/
│
├── core/                                (2 个文件 — 核心数据)
│   └── ProjectDataSource.h / .cpp      310行
│
└── ui/                                 (8 个文件 — 页面组件)
    ├── WelcomePage.h / .cpp            266行
    ├── NewProjectDialog.h / .cpp       173行
    ├── DsSlicerPage.h / .cpp           555行
    └── ExportPage.h / .cpp           ~1050行 (内部重构消除重复)
```

### 2.3 变更详述

#### 变更 A 🔴 删除 `DsLabelerKeys.h`（32行，零引用）

```cpp
// 删除内容 — 全代码库零引用
namespace DsLabelerKeys {
    using dsfw::CommonKeys::LastDir;
    // ... 15 个 SettingsKey<QString> 定义 ...
}
```

> 如果这些快捷键定义未来需要用到，应在用到时再加入，而非保留死代码。

#### 变更 B 🔴 删除 `LayerDependencyGraph.h/.cpp`（113行，零引用）

该类定义了 `grapheme→phoneme→ph_num→midi` 依赖边和脏标记传播逻辑，但从未在任何管线中被实例化或调用。

> 如果 ADR-57 的 dirty 传播未来需要实现，届时再用 `core/` 下的新文件承载。

#### 变更 C 🟡 ExportPage 内部重构 — 消除 ~150 行重复推理代码

将 `autoCompleteSlice()` 和批量 lambda 的公共推理逻辑合并为一个私有方法：

```cpp
// ExportPage.h 新增私有方法声明
private:
    struct AutoCompleteContext {
        bool hasGrapheme = false;
        bool hasPhoneme = false;
        bool hasPhNum = false;
        bool hasPitch = false;
        bool hasMidi = false;
        QStringList graphemeTexts;
        QStringList phonemeTexts;
    };

    /// 收集当前文档的层状态
    AutoCompleteContext scanLayers(const DsTextDocument &doc) const;

    /// 对单个文档执行自动补全（纯逻辑，无副作用，可在任意线程调用）
    /// @return 修改后的文档和是否变更标志
    struct AutoCompleteOutcome {
        DsTextDocument doc;
        bool modified = false;
    };
    AutoCompleteOutcome runAutoComplete(DsTextDocument doc,
                                         const QString &audioPath,
                                         HFA::HFA *hfa,
                                         Rmvpe::Rmvpe *rmvpe,
                                         Game::Game *game,
                                         PhNumCalculator *phNumCalc);
```

**重构后的代码流：**

```cpp
// autoCompleteSlice() 主线程版 → 简化为：
void ExportPage::autoCompleteSlice(const QString &sliceId) {
    auto result = m_source->loadSlice(sliceId);
    if (!result) return;
    auto outcome = runAutoComplete(std::move(result.value()),
                                    m_source->audioPath(sliceId),
                                    m_hfa.get(), m_rmvpe.get(),
                                    m_game.get(), m_phNumCalc.get());
    if (outcome.modified)
        (void)m_source->saveSlice(sliceId, outcome.doc);
}

// 批量 lambda 后台线程版 → 简化为：
QtConcurrent::run([...]() {
    for (const auto &sliceId : sliceIds) {
        auto result = src->loadSlice(sliceId);
        if (!result) continue;
        auto outcome = runAutoComplete(std::move(result.value()),
                                        src->audioPath(sliceId),
                                        hfa, rmvpe, game, phNumCalc);
        if (outcome.modified) {
            std::lock_guard lock(mutex);
            modifiedDocs[sliceId] = std::move(outcome.doc);
        }
        // ... 进度回调不变 ...
    }
});
```

> **效果**：消除 ~150 行重复推理代码，ExportPage 从 1288 行缩减至 ~1050 行。

#### 变更 D 🟡 main.cpp 精简 — SliceConsistencyGuard 提取为命名空间函数

将一致性校验从 `currentPageChanged` lambda 提取到 main.cpp 顶部的命名空间自由函数（不新增文件）：

```cpp
// main.cpp 顶部新增（~5行封装）
namespace {

/// 检查切片一致性，不一致时弹窗并可能跳回切片页
void checkSliceConsistency(dstools::DsProject *project,
                           dstools::ProjectDataSource *dataSource,
                           QWidget *parent);

} // namespace

// main() 中的 currentPageChanged lambda 从 ~115行 → ~5行：
QObject::connect(&shell, &dsfw::AppShell::currentPageChanged, &shell,
    [&](int index) {
        if (index < 2 || index > 4) return;
        if (!project || !dataSource) return;
        checkSliceConsistency(project.get(), dataSource, &shell);
    });
```

> **效果**：main.cpp 从 287 行精简至 ~170 行，逻辑清晰但不新增文件。

---

## 3 变更总览

| 操作 | 文件 | 行数变化 | 原因 |
|------|------|---------|------|
| 🔴 删除 | `DsLabelerKeys.h` | -32 | 零引用死代码 |
| 🔴 删除 | `LayerDependencyGraph.h/.cpp` | -113 | 零引用死代码 |
| 🟡 重构 | `ExportPage` 内部 | -150 (重复消除) | autoCompleteSlice / batch lambda 合并 |
| 🟡 精简 | `main.cpp` | -117 (提取函数) | SliceConsistencyGuard 改为命名空间函数 |
| 🔵 移动 | `ProjectDataSource` → `core/` | 0 | 分层：数据 vs UI |
| 🔵 移动 | 4 个 Page → `ui/` | 0 | 分层 |

| 指标 | 重构前 | 重构后 |
|------|--------|--------|
| 源文件数 | 15 | 12 |
| 总代码行 | ~2965 | ~2685 |
| 目录层级 | 1（平铺） | 2（core/ + ui/） |
| 重复代码 | ~150 行 | 0 |
| 死代码 | 145 行 | 0 |
| core/ 文件数 | — | 2 |
| ui/ 文件数 | — | 8 |

---

## 4 CMakeLists.txt 更新

```cmake
project(DsLabeler VERSION 0.1.0.0)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent REQUIRED)
find_package(cpp-pinyin CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

dstools_add_executable(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    AUTOMOC AUTORCC
    DEPLOY WIN32_EXECUTABLE MACOSX_BUNDLE WINRC
    # ── 源文件 ──
    main.cpp
    core/ProjectDataSource.h core/ProjectDataSource.cpp
    ui/WelcomePage.h ui/WelcomePage.cpp
    ui/NewProjectDialog.h ui/NewProjectDialog.cpp
    ui/DsSlicerPage.h ui/DsSlicerPage.cpp
    ui/ExportPage.h ui/ExportPage.cpp
    DEPENDS
        PRIVATE dstools-ui-core dstools-widgets dstools-domain
                min-label-editor phoneme-editor pitch-editor settings-page
                data-sources log-page
                lyricfa-lib hubertfa-lib rmvpepitch-lib gameinfer-lib
                SndFile::sndfile
                Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent
)

dstools_add_translations(${PROJECT_NAME})

# cpp-pinyin dict copy 逻辑不变...
```

---

## 5 include 路径变更

| 文件 | 原 | 新 |
|------|----|----|
| main.cpp | `#include "WelcomePage.h"` | `#include "ui/WelcomePage.h"` |
| main.cpp | `#include "ExportPage.h"` | `#include "ui/ExportPage.h"` |
| main.cpp | `#include "DsSlicerPage.h"` | `#include "ui/DsSlicerPage.h"` |
| main.cpp | `#include "ProjectDataSource.h"` | `#include "core/ProjectDataSource.h"` |
| main.cpp | `#include "DsLabelerKeys.h"` | **删除（死代码）** |
| WelcomePage.cpp | `#include "NewProjectDialog.h"` | `#include "ui/NewProjectDialog.h"` |
| DsSlicerPage.cpp | `#include "ProjectDataSource.h"` | `#include "core/ProjectDataSource.h"` |
| ExportPage.cpp | `#include "ProjectDataSource.h"` | `#include "core/ProjectDataSource.h"` |

---

## 6 设计准则核对

| 准则 | 满足方式 |
|------|---------|
| **P-01** 模块职责单一 | 死代码消除；ExportPage 推理逻辑不再重复出现在两个位置 |
| **P-03** 异步一切 | `autoCompleteSlice()` 主线程同步调用推理的问题被标注（建议后续改为统一走异步路径） |
| **P-12** 相似模块统一 | DsSlicerPage 继承 SlicerPage 保持不变 |
| **P-14** 组合优于继承 | 不变，各 Page 组合使用 ProjectDataSource |
| **P-16** 开闭原则 | 重构通过合并现有方法实现，未修改公共接口 |

> ⚠ **遗留建议**：`autoCompleteSlice()` 中含推理调用（HFA/RMVPE/GAME），在主线程同步执行违反 P-03。建议后续改为通过 QtConcurrent::run 异步执行（复用重构后的 `runAutoComplete`）。

---

## 7 任务分解

| 编号 | 任务 | 说明 | 风险 |
|------|------|------|------|
| **R-01** | 删除 `DsLabelerKeys.h` | 确认全代码库零引用后删除 | 低 |
| **R-02** | 删除 `LayerDependencyGraph.h/.cpp` | 确认全代码库零引用后删除 | 低 |
| **R-03** | 创建 `core/` 目录，移动 `ProjectDataSource` | 更新 include 路径 | 低 |
| **R-04** | 创建 `ui/` 目录，移动 4 组 Page 文件 | 更新 include 路径 | 低 |
| **R-05** | 更新 `CMakeLists.txt` | 源文件路径 + 确认 include dir | 低 |
| **R-06** | **编译验证** | 确保 R-01~R-05 后编译通过 | — |
| **R-07** | ExportPage 内部重构 | 提取 `runAutoComplete()`，合并两份推理逻辑 | 中 |
| **R-08** | main.cpp 精简 | SliceConsistencyGuard 改为命名空间函数 | 低 |
| **R-09** | 最终编译 + 冒烟测试 | 全量验证 | — |

> 执行顺序：R-01→R-06（目录重排 + 删死代码，先确保可编译）→ R-07→R-08（重构去重）→ R-09（验证）

---

## 8 关联文档

- [human-decisions.md](../human-decisions.md) — 设计准则与决策记录
- [framework-architecture.md](framework-architecture.md) — 六层架构
- [unified-app-design.md](unified-app-design.md) — DsLabeler 页面设计
- [refactoring-plan.md](../refactoring-plan.md) — 重构任务清单