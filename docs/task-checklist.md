# 框架抽取实施路线图（细化版）

> **总工时**: 22 ~ 33 天（单人）  
> **关键路径**: T-01 → T-03 → T-04 → T-05 → T-06 → T-09 → T-10 → T-11 → T-12 → T-22 → T-27 → T-28  
> **最大并行度**: 4（B-3 批次可同时推进 4 条独立任务线）  
> **🔴高风险任务数**: 4 | **🟡中风险任务数**: 20 | **🟢低风险任务数**: 22

---

## 术语和图例

| 图标 | 含义 |
|------|------|
| 🟢 | 低风险 — 操作机械、影响面小 |
| 🟡 | 中风险 — 需要判断或影响面较大 |
| 🔴 | 高风险 — 可能导致大面积返工或阻塞 |
| ⏩ | 可与同批次其他任务并行 |
| ⛓️ | 串行，必须等前置任务完成 |
| B-N | 执行批次编号，同批次内的任务可并行 |

---

## 总览：执行批次与关键路径（ASCII 甘特图）

```
批次    Week1           Week2           Week3           Week4           Week5
       D1  D2  D3  D4  D5  D6  D7  D8  D9  D10 D11 D12 D13 D14 D15 D16~D22+
       ─── ─── ─── ─── ─── ─── ─── ─── ─── ─── ─── ─── ─── ─── ─── ───────

B-1    [T01][T02]                                                      ← 准备
B-2    ····[T-03 ]                                                     ← CMake骨架
B-3         ····[T-04]                                                 ← domain库
       ····[===T-15===]                                                ← IPageActions(可并行)
       ····[T-29][T-30][T-31]                                          ← pipeline独立化(可并行)
       ····[T-34]                                                      ← 命名空间化(可并行)
B-4              [=====T-05=====]                                      ← 通用类迁移 ★关键
B-5                          [T-06][T-07]                              ← 领域类迁移
                        [T-16~T-20 ]                                   ← Page实现
                             [T-21 ]                                   ← Shortcut隔离
B-6                               [T-08][T-09 ]                       ← 依赖清理+include
                                  [T-32][T-33 ]                        ← WorkThread+验证
                                  [====T-22====]                       ← AppShell ★关键
B-7                                      [T-10][T-11 ]                 ← 旧core删除+验证
                                         [T-35][T-36]                  ← CMake export
B-8                                              [T-12~T-14]           ← UI-Core清理
                                                 [T-23~T-26]           ← 单页迁移
                                                 [T-37][T-38]          ← install+验证
B-9                                                    [===T-27===]    ← DSLabeler迁移 ★关键
                                                       [T-39~T-42]     ← 测试
B-10                                                        [T-28]     ← 清理回归
                                                            [T-43~T-46]← 文档

关键路径(粗线): T-01→T-03→T-04→T-05→T-06→T-09→T-10→T-11→T-12→T-22→T-27→T-28
                ═══════════════════════════════════════════════════════════════
```

---

## Phase 0: 准备工作（1 ~ 2 天）

### T-01: ✅ 创建分支与快照 🟢 ⏩
**耗时**: 0.2d | **依赖**: 无 | **执行批次**: B-1

**步骤**:
1. `git checkout -b refactor/extract-framework main`
2. `git tag pre-refactor-snapshot`
3. 推送分支，确认 CI 三平台构建通过（检查 `.github/workflows/`）

**风险说明**: 无实质风险，纯版本控制操作。

**产出物**: `refactor/extract-framework` 分支 + `pre-refactor-snapshot` tag

**检验方式**: CI 三平台（Windows/macOS/Linux）构建绿灯

---

### T-02: ✅ 确定框架命名空间 🟢 ⏩
**耗时**: 0.3d | **依赖**: T-01 | **执行批次**: B-1

**步骤**:
1. 确定根命名空间 `dsfw`（区别于现有 `dstools`）
2. 在 `src/core/include/dstools/` 下新增 `dsfw/` 占位目录
3. 编写 `docs/NAMESPACE_MIGRATION.md`，记录旧→新命名空间映射表

**风险说明**: 命名空间决策一旦确定后续难以更改，但影响面仅为文本替换。

**产出物**: `docs/NAMESPACE_MIGRATION.md` + 命名空间设计决定

**检验方式**: 文档存在且映射表覆盖所有已知框架类（12+）

---

### T-03: ✅ 搭建 CMake 骨架 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-02 | **执行批次**: B-2

**步骤**:
1. 新建目录结构 `src/framework/{core,ui-core}/` 各含 `CMakeLists.txt` + `include/dsfw/` + `src/`
2. 声明 `dsfw-core` 和 `dsfw-ui-core` 两个 STATIC 库（空壳）
3. 修改 `src/CMakeLists.txt` 添加 `add_subdirectory(framework)` 并编译验证

**风险说明**: CMake 骨架的目录结构和 target 命名会影响后续所有任务。错误的设计会导致 Phase 1~4 全部返工。需仔细评审 target 依赖链。

**产出物**: 空壳 `dsfw-core` / `dsfw-ui-core` 可编译

**检验方式**: `cmake --build build` 通过（三平台），两个 target 存在于 build 产物中

**涉及文件**:
- `src/CMakeLists.txt`
- `src/framework/CMakeLists.txt`（新建）
- `src/framework/core/CMakeLists.txt`（新建）
- `src/framework/ui-core/CMakeLists.txt`（新建）

---

### ✅ Phase 0 检查点
- [x] 分支存在且 CI 通过
- [x] 命名空间文档完备
- [x] 空壳库三平台编译通过
- [x] 目录结构与设计文档一致

---

## Phase 1: Core 层分离（3 ~ 5 天）

### T-04: ✅ 创建 domain 静态库 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-03 | **执行批次**: B-3

**步骤**:
1. 新建 `src/domain/CMakeLists.txt`，声明 `dstools-domain` STATIC 库
2. 设置链接: `target_link_libraries(dstools-domain PUBLIC dsfw-core Qt6::Core Qt6::Network nlohmann_json::nlohmann_json)`
3. 修改 `src/CMakeLists.txt` 添加 `add_subdirectory(domain)`，编译验证

**风险说明**: 简单操作，仅创建空壳。

**产出物**: `dstools-domain` 空壳静态库可编译

**检验方式**: `cmake --build build --target dstools-domain` 通过

**涉及文件**:
- `src/domain/CMakeLists.txt`（新建）
- `src/domain/include/dstools/`（新建空目录）
- `src/domain/src/`（新建空目录）
- `src/CMakeLists.txt`

---

### T-05: 迁移通用类到框架 🟡 ⛓️
**耗时**: 1.5d | **依赖**: T-04 | **执行批次**: B-4

逐文件搬迁，每搬一个类编译验证。按风险从低到高排序：

#### T-05a: AppSettings 🟢
- 搬迁 `src/core/include/dstools/AppSettings.h` → `src/framework/core/include/dsfw/AppSettings.h`
- 搬迁 `src/core/src/AppSettings.cpp` → `src/framework/core/src/AppSettings.cpp`
- 纯通用设置管理，无领域依赖

#### T-05b: JsonHelper 🟢
- 搬迁 `src/core/include/dstools/JsonHelper.h` → `src/framework/core/include/dsfw/JsonHelper.h`
- 搬迁 `src/core/src/JsonHelper.cpp` → `src/framework/core/src/JsonHelper.cpp`
- 纯 JSON 工具函数，无领域依赖

#### T-05c: AsyncTask 🟢
- 搬迁 `src/core/include/dstools/AsyncTask.h` → `src/framework/core/include/dsfw/AsyncTask.h`
- 搬迁 `src/core/src/AsyncTask.cpp` → `src/framework/core/src/AsyncTask.cpp`
- 通用异步任务抽象

#### T-05d: ServiceLocator 🟡
- 搬迁 `src/core/include/dstools/ServiceLocator.h` → `src/framework/core/include/dsfw/ServiceLocator.h`
- 搬迁 `src/core/src/ServiceLocator.cpp` → `src/framework/core/src/ServiceLocator.cpp`
- **需清理**: 遗留 `fileIO()` 便捷 API 可能耦合领域类型，需评估是否保留或改为纯泛型

#### T-05e: IDocument / IFileIOProvider / LocalFileIOProvider / IExportFormat 🟡
- 搬迁 6 个头文件 + 1 个源文件到 `src/framework/core/`
- `IDocument.h` 需重构: `DocumentFormat` 枚举含领域值（DsProject/TextGrid），需抽象为泛型或移除枚举
- `IFileIOProvider.h` / `LocalFileIOProvider.h/.cpp` 纯通用

#### T-05f: IModelProvider / IModelDownloader / IQualityMetrics / IG2PProvider 🟡
- 搬迁 4 个接口头文件到 `src/framework/core/include/dsfw/`
- `IModelProvider.h` 需重构: `ModelType` 枚举含 DiffSinger 特定值，需改为字符串或泛型枚举

#### T-05g: ModelManager 拆分 🔴
- 从 `src/core/src/ModelManager.cpp` 提取通用 `IModelRegistry` 接口到 `dsfw/`
- DiffSinger 特定的模型注册逻辑（ASR/HuBERT/RMVPE/GAME 模型路径）留在 `src/domain/`
- **风险**: 混合通用/领域逻辑，拆分边界需要仔细判断；拆错会导致循环依赖

#### T-05h: ModelDownloader 🟡
- 通用下载逻辑（HTTP 下载 + 进度回调）搬到 `dsfw/`
- 模型 URL 配置和版本检测逻辑留在 `src/domain/`
- **风险**: 下载逻辑通用但 URL 路由是领域的，需定义清晰的回调接口

**步骤总结**:
1. 按 T-05a~T-05h 顺序逐个搬迁，每次更新 `src/framework/core/CMakeLists.txt` 的 SOURCES/HEADERS
2. 每搬一个类立即 `cmake --build` 验证无编译错误
3. 对 T-05d~T-05h 中需重构的类，先创建接口/抽象层再搬迁实现

**风险说明**: ModelManager 拆分是本任务最大风险点。拆分不当会导致框架层反向依赖领域层，形成循环依赖。建议先画依赖图再动手。

**产出物**: `dsfw-core` 包含 12+ 头文件、5+ 源文件，可独立编译

**检验方式**: `cmake --build build --target dsfw-core` 通过；`dsfw-core` 不依赖任何 `dstools` 命名空间的类型

**涉及文件**:
- `src/framework/core/CMakeLists.txt`（持续更新）
- `src/framework/core/include/dsfw/*.h`（新建 12+ 头文件）
- `src/framework/core/src/*.cpp`（新建 5+ 源文件）
- `src/core/include/dstools/`（删除已搬迁文件）
- `src/core/src/`（删除已搬迁文件）

---

### T-06: 迁移领域类到 domain 库 🟢 ⛓️
**耗时**: 1d | **依赖**: T-05 | **执行批次**: B-5

**步骤**:
1. 将 14 个领域头文件从 `src/core/include/dstools/` 搬到 `src/domain/include/dstools/`（DsDocument、DsProject、DsItemManager、CsvToDsConverter、TextGridToCsv、TranscriptionCsv、TranscriptionPipeline、PitchUtils、F0Curve、PhNumCalculator、AudioFileResolver、PinyinG2PProvider、ExportFormats、DsDocumentAdapter）
2. 将 13 个源文件从 `src/core/src/` 搬到 `src/domain/src/`
3. 更新 `src/domain/CMakeLists.txt` 的 SOURCES/HEADERS 列表，编译验证

**风险说明**: 纯机械搬迁，文件路径变更但代码不变。风险极低。

**产出物**: `dstools-domain` 包含所有领域类，可编译

**检验方式**: `cmake --build build --target dstools-domain` 通过

**涉及文件**:
- `src/domain/CMakeLists.txt`
- `src/domain/include/dstools/*.h`（14 个）
- `src/domain/src/*.cpp`（13 个）

---

### T-07: CommonKeys 拆分 🟡 ⏩
**耗时**: 0.3d | **依赖**: T-05 | **执行批次**: B-5

**步骤**:
1. 将 `ThemeMode`、`LastDir` 等通用 key 提取到 `src/framework/core/include/dsfw/CommonKeys.h`
2. 将 DiffSinger 特定 key（快捷键、模型路径等）保留在 `src/domain/include/dstools/CommonKeys.h`
3. 两个文件均依赖 `dsfw/AppSettings.h` 中的 `SettingsKey<>` 模板

**风险说明**: 分界线不清晰时可能遗漏 key，导致运行时找不到设置项。需逐一审查每个 key 的使用者。

**产出物**: 两个 `CommonKeys.h` 文件各自独立可编译

**检验方式**: 全量编译通过；grep 确认无未解析的 CommonKeys 引用

**涉及文件**:
- `src/framework/core/include/dsfw/CommonKeys.h`（新建）
- `src/domain/include/dstools/CommonKeys.h`（修改）
- `src/core/include/dstools/CommonKeys.h`（删除）

---

### T-08: 移除 textgrid/SndFile 依赖到 domain 🟢 ⛓️
**耗时**: 0.2d | **依赖**: T-06 | **执行批次**: B-6

**步骤**:
1. 从 `src/framework/core/CMakeLists.txt` 确认无 `textgrid` / `SndFile` 链接（本不该有）
2. 在 `src/domain/CMakeLists.txt` 添加 `find_package(SndFile CONFIG REQUIRED)` + `target_link_libraries(dstools-domain PRIVATE textgrid SndFile::sndfile)`
3. 确认 `TextGridToCsv` 和 `AudioFileResolver` 已在 domain 中

**风险说明**: 操作简单，仅移动 CMake 链接声明。

**产出物**: `dsfw-core` 无第三方领域依赖

**检验方式**: `dsfw-core` 的 CMakeLists.txt 中无 textgrid/SndFile；domain 编译通过

**涉及文件**:
- `src/domain/CMakeLists.txt`
- `src/core/CMakeLists.txt`（移除旧链接）

---

### T-09: 更新所有 include 路径 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-06, T-07 | **执行批次**: B-6

**步骤**:
1. 全局搜索替换 12 个框架类的 include：`#include <dstools/XXX.h>` → `#include <dsfw/XXX.h>`
2. 确认领域类 include 路径保持 `<dstools/XXX.h>` 不变
3. （可选）在旧路径放置 `#pragma once` + `#include <dsfw/XXX.h>` 转发头文件

**风险说明**: 影响面大（全部 `src/apps/`、`src/ui-core/`、`src/widgets/`、`src/domain/`），但操作机械。主要风险是遗漏替换导致编译失败，可通过全量编译发现。

**产出物**: 全部 include 路径更新完毕

**检验方式**: `cmake --build build` 全量通过；grep `#include <dstools/AppSettings.h>` 返回 0 结果（或仅转发头文件）

**涉及文件**:
- `src/apps/` 下所有 `.cpp`、`.h`
- `src/ui-core/src/*.cpp`、`src/ui-core/include/dstools/*.h`
- `src/widgets/` 下所有源文件
- `src/domain/` 下所有源文件

---

### T-10: 删除/降级旧 dstools-core 🟡 ⛓️
**耗时**: 0.3d | **依赖**: T-09 | **执行批次**: B-7

**步骤**:
1. 选择策略：直接删除 `src/core/` 或降级为薄壳（仅 `target_link_libraries(dstools-core PUBLIC dsfw-core dstools-domain)`）
2. 更新所有上层 `target_link_libraries` 从 `dstools-core` 改为 `dsfw-core` + `dstools-domain`
3. 更新 `src/CMakeLists.txt` 移除/修改 `add_subdirectory(core)`

**风险说明**: 删除核心库是高影响操作。若有遗漏的依赖者会立即编译失败。建议先降级为薄壳，确认无问题后再删除。

**产出物**: `src/core/` 删除或降级为空壳

**检验方式**: 全量编译通过；`src/core/` 不再包含实质代码

**涉及文件**:
- `src/core/CMakeLists.txt`（删除或简化）
- `src/CMakeLists.txt`
- 所有引用 `dstools-core` 的 CMakeLists.txt

---

### T-11: 全量编译验证 🔴 ⛓️
**耗时**: 0.5d | **依赖**: T-10 | **执行批次**: B-7

**步骤**:
1. Windows (MSVC 2022) 全量 `cmake --build build`
2. macOS (Clang) + Linux (GCC) 全量编译
3. 运行已有测试 `TestAudioUtil`、`TestGame`、`TestRmvpe`

**风险说明**: Phase 1 所有问题在此集中暴露。可能出现：循环依赖、MOC 未处理 Q_OBJECT 头文件、链接顺序错误、符号未导出等。预留充足调试时间。

**产出物**: 三平台编译绿灯 + 测试通过

**检验方式**: CI 三平台全绿；`ctest --test-dir build` 全部 PASS

---

### ✅ Phase 1 检查点
- [ ] `dsfw-core` 包含全部 12+ 通用类，独立编译通过
- [ ] `dstools-domain` 包含全部 14 领域类，编译通过
- [ ] 旧 `src/core/` 已删除或降级
- [ ] 三平台全量编译通过
- [ ] 已有测试无回归
- [ ] 无循环依赖（可用 `cmake --graphviz` 验证）

---

## Phase 2: UI-Core 清理（1 天）

### T-12: 移除 cpp-pinyin 硬依赖 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-11 | **执行批次**: B-8

**步骤**:
1. 在 `dsfw-ui-core` 的 `AppInit.h` 中定义 `InitHook` 回调机制：`using InitHook = std::function<void()>; void registerPostInitHook(InitHook hook);`
2. 将 `AppInit.cpp` 中 cpp-pinyin 字典加载代码提取到 `src/domain/src/DsAppInit.cpp`，通过 `registerPostInitHook` 注册
3. 从 `src/framework/ui-core/CMakeLists.txt` 移除 cpp-pinyin 链接；在 `src/domain/CMakeLists.txt` 添加 cpp-pinyin 依赖

**风险说明**: 若 AppInit 初始化顺序被打乱，可能导致 pinyin 字典未加载时 G2P 调用崩溃。需确保 hook 在应用 main() 早期被注册。

**产出物**: `dsfw-ui-core` 不再依赖 cpp-pinyin

**检验方式**: `dsfw-ui-core` 的 CMake 依赖列表中无 cpp-pinyin；MinLabel 启动后 G2P 功能正常

**涉及文件**:
- `src/framework/ui-core/CMakeLists.txt`
- `src/framework/ui-core/include/dsfw/AppInit.h`
- `src/framework/ui-core/src/AppInit.cpp`
- `src/domain/src/DsAppInit.cpp`（新建）
- `src/domain/CMakeLists.txt`

---

### T-13: 迁移 UI-Core 通用类到框架 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-12 | **执行批次**: B-8

**步骤**:
1. 搬迁 `Theme.h/.cpp`、`FramelessHelper.h/.cpp`、`IStepPlugin.h`、`AppInit.h/.cpp` 到 `src/framework/ui-core/`
2. 更新 `src/framework/ui-core/CMakeLists.txt` SOURCES/HEADERS
3. 编译验证

**风险说明**: 机械搬迁，风险低。

**产出物**: `dsfw-ui-core` 包含完整 UI 通用类

**检验方式**: `cmake --build build --target dsfw-ui-core` 通过

**涉及文件**:
- `src/framework/ui-core/include/dsfw/Theme.h`（搬迁）
- `src/framework/ui-core/include/dsfw/FramelessHelper.h`（搬迁）
- `src/framework/ui-core/include/dsfw/IStepPlugin.h`（搬迁）
- `src/framework/ui-core/src/Theme.cpp`（搬迁）
- `src/framework/ui-core/src/FramelessHelper.cpp`（搬迁）
- `src/framework/ui-core/CMakeLists.txt`

---

### T-14: 验证 CommonKeys 在 UI 层的引用 🟢 ⛓️
**耗时**: 0.2d | **依赖**: T-13 | **执行批次**: B-8

**步骤**:
1. 确认 `Theme.cpp` 中 `CommonKeys::ThemeMode` 引用指向 `dsfw/CommonKeys.h`
2. 确认各应用入口文件 include 路径正确
3. 全量编译验证

**风险说明**: 可能有遗漏的 CommonKeys 引用，编译时即可发现。

**产出物**: UI-Core 层 CommonKeys 引用全部正确

**检验方式**: 全量编译通过；grep 确认无错误引用

---

### ✅ Phase 2 检查点
- [ ] `dsfw-ui-core` 无领域依赖（无 cpp-pinyin、无 DiffSinger 特定代码）
- [ ] 全量编译通过
- [ ] MinLabel/PhonemeLabeler 启动正常（验证 Theme 和 AppInit）

---

## Phase 2.5: 统一壳 AppShell 实现与迁移（7 ~ 10 天）

### T-15: ✅ IPageActions 接口扩展 🟡 ⏩
**耗时**: 1d | **依赖**: 无（可与 Phase 1/2 并行） | **执行批次**: B-3

**步骤**:
1. 在 `src/apps/labeler-interfaces/include/IPageActions.h` 新增方法：`createMenuBar(QWidget*)`, `shortcutManager()`, `createStatusBarContent(QWidget*)`, `windowTitle()`, `supportsDragDrop()`/`handleDragEnter()`/`handleDrop()`
2. 所有新方法提供默认实现（返回 nullptr/空串/false），确保现有 Page 无需立即修改
3. 将旧方法 `editActions()`/`viewActions()`/`toolActions()` 标记 `[[deprecated]]`

**风险说明**: 接口变更影响所有 IPageActions 实现类。但由于提供默认实现，现有代码不会立即中断。后续 T-16~T-20 需逐个实现新方法。

**产出物**: `IPageActions.h` 扩展完毕，编译通过

**检验方式**: 全量编译通过；现有 Page 类无需修改即可编译

**涉及文件**:
- `src/apps/labeler-interfaces/include/IPageActions.h`

---

### T-16: MinLabelPage 实现完整接口 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-15 | **执行批次**: B-5

**步骤**:
1. 从 `src/apps/MinLabel/gui/MainWindow.cpp` 的 `buildMenuBar()` 提取菜单构建逻辑 → 实现 `MinLabelPage::createMenuBar()`
2. 从 `MainWindow::dropEvent()` 提取 → 实现 `handleDrop()`；从状态栏构建中提取 → `createStatusBarContent()`
3. 实现 `windowTitle()` 返回 "MinLabel"；编译验证

**风险说明**: 菜单栏中 QAction 的 parent 关系需要从 MainWindow 迁移到新的 parent widget，可能触发内存管理问题。

**产出物**: `MinLabelPage` 完整实现 IPageActions 新接口

**检验方式**: MinLabel 应用编译通过；菜单栏在 AppShell 迁移前仍正常工作

**涉及文件**:
- `src/apps/MinLabel/gui/MinLabelPage.h`
- `src/apps/MinLabel/gui/MinLabelPage.cpp`
- `src/apps/MinLabel/gui/MainWindow.cpp`（参考提取）

---

### T-17: PhonemeLabelerPage 实现完整接口 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-15 | **执行批次**: B-5

**步骤**:
1. 从 `src/apps/PhonemeLabeler/gui/MainWindow.cpp` 提取 `buildMenuBar()` → `createMenuBar()`
2. 从 `MainWindow::buildStatusBar()` 提取 → `createStatusBarContent()`（含进度信息）
3. 实现 `windowTitle()` 返回 "PhonemeLabeler"；编译验证

**风险说明**: 状态栏内容可能引用 MainWindow 成员变量，需重构为 Page 内部状态。

**产出物**: `PhonemeLabelerPage` 完整实现 IPageActions

**检验方式**: PhonemeLabeler 编译通过

**涉及文件**:
- `src/apps/PhonemeLabeler/gui/PhonemeLabelerPage.h`
- `src/apps/PhonemeLabeler/gui/PhonemeLabelerPage.cpp`

---

### T-18: PitchLabelerPage 实现完整接口 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-15 | **执行批次**: B-5

**步骤**:
1. 从 `src/apps/PitchLabeler/gui/MainWindow.cpp` 提取 `buildMenuBar()` → `createMenuBar()`
2. 实现 `createStatusBarContent()` + `windowTitle()` 返回 "PitchLabeler"
3. 编译验证

**风险说明**: 同 T-16，QAction parent 迁移。

**产出物**: `PitchLabelerPage` 完整实现 IPageActions

**检验方式**: PitchLabeler 编译通过

**涉及文件**:
- `src/apps/PitchLabeler/gui/PitchLabelerPage.h`
- `src/apps/PitchLabeler/gui/PitchLabelerPage.cpp`

---

### T-19: 创建 GameInferPage 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-15 | **执行批次**: B-5

**步骤**:
1. 新建 `src/apps/GameInfer/gui/GameInferPage.h/.cpp`，继承 IPageActions
2. 从 `src/apps/GameInfer/gui/MainWidget.cpp` 提取核心 UI 逻辑到 Page
3. 从 `src/apps/GameInfer/gui/MainWindow.cpp` 提取菜单/拖拽逻辑 → 实现新接口方法

**风险说明**: 新建类，需从两个现有类（MainWidget + MainWindow）整合逻辑。整合过程中可能遗漏信号槽连接。

**产出物**: `GameInferPage` 实现完整 IPageActions

**检验方式**: GameInfer 编译通过

**涉及文件**:
- `src/apps/GameInfer/gui/GameInferPage.h`（新建）
- `src/apps/GameInfer/gui/GameInferPage.cpp`（新建）
- `src/apps/GameInfer/CMakeLists.txt`

---

### T-20: Pipeline 页面适配 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-15 | **执行批次**: B-5

**步骤**:
1. 确认 `TaskWindowAdapter` 已基本实现 IPageActions（已有基础）
2. 补充 `createMenuBar()` 返回 Pipeline 标准菜单
3. BuildCsvPage/BuildDsPage 补充基础 `createMenuBar()`

**风险说明**: TaskWindowAdapter 已有框架基础，适配工作量小。

**产出物**: Pipeline 所有页面支持 IPageActions

**检验方式**: DatasetPipeline 编译通过

**涉及文件**:
- `src/apps/labeler/TaskWindowAdapter.h`
- `src/apps/labeler/TaskWindowAdapter.cpp`

---

### T-21: ShortcutManager 作用域隔离 🟡 ⏩
**耗时**: 0.5d | **依赖**: T-15 | **执行批次**: B-5

**步骤**:
1. 在 `src/widgets/include/dstools/ShortcutManager.h` 新增 `setEnabled(bool)` 方法
2. 实现: 遍历所有注册的 QShortcut，调用 `setEnabled(enabled)`
3. 在 `IPageLifecycle::onActivated()` 中启用、`onDeactivated()` 中禁用；验证不同页面相同快捷键不冲突

**风险说明**: QShortcut 在 QStackedWidget 中的 context 行为需验证。`Qt::WidgetWithChildrenShortcut` 可能在页面隐藏时仍触发。需测试并可能改用 `Qt::WindowShortcut` + 手动管理。

**产出物**: ShortcutManager 支持按页面启用/禁用

**检验方式**: 编译通过；手动测试多页面快捷键无冲突

**涉及文件**:
- `src/widgets/include/dstools/ShortcutManager.h`
- `src/widgets/src/ShortcutManager.cpp`

---

### T-22: 实现 AppShell 框架组件 🔴 ⛓️
**耗时**: 2d | **依赖**: T-15, T-21 | **执行批次**: B-6

**步骤**:
1. 新建 `src/widgets/include/dstools/AppShell.h` + `src/widgets/src/AppShell.cpp`
2. AppShell(QMainWindow) 内部组合: FramelessHelper + TitleBar(自绘) + IconNavBar(可选,40-48px) + QStackedWidget(PageHost) + QStatusBar
3. 实现核心 API:
   - `addPage(IPageActions *page, const QString &icon, const QString &tooltip)` — 注册页面
   - 页面切换: `setMenuBar(page->createMenuBar())` + `shortcutManager()->setEnabled()` + 更新标题栏 + 更新状态栏
   - 单页面模式自动隐藏 IconNavBar
   - `setGlobalFileActions(QList<QAction*>)` — 全局操作注入到每个菜单栏
   - `closeEvent`: 检查所有页面 `hasUnsavedChanges()`
   - 拖拽转发: `dragEnterEvent`/`dropEvent` → 当前页面 `supportsDragDrop()` → `handleDrop()`

**风险说明**: 这是整个重构的核心组件。窗口框架层变更影响所有 6 个应用的视觉和交互。FramelessHelper 在不同平台行为不一致（Windows DWM vs macOS NSWindow vs Linux X11/Wayland）。菜单栏动态切换可能闪烁。需要在三平台充分测试。

**产出物**: `AppShell` 类可编译且单元可测试

**检验方式**: 编译通过；编写最小 demo（单页面 + 多页面模式）手动验证

**涉及文件**:
- `src/widgets/include/dstools/AppShell.h`（新建）
- `src/widgets/src/AppShell.cpp`（新建）
- `src/widgets/CMakeLists.txt`（更新）

---

### T-23: 迁移 GameInfer 到 AppShell 🟡 ⛓️
**耗时**: 0.3d | **依赖**: T-19, T-22 | **执行批次**: B-8

**步骤**:
1. 修改 `src/apps/GameInfer/main.cpp`: 创建 AppShell → `addPage(new GameInferPage(...))`
2. 删除 `src/apps/GameInfer/gui/MainWindow.h/.cpp`
3. 验证: 启动、推理、拖拽文件

**风险说明**: 首个迁移，验证 AppShell 实际可行性。若此处失败需回退修改 AppShell 设计。

**产出物**: GameInfer 使用 AppShell 运行

**检验方式**: GameInfer.exe 启动正常；拖拽音频文件可推理

**涉及文件**:
- `src/apps/GameInfer/main.cpp`
- `src/apps/GameInfer/gui/MainWindow.h`（删除）
- `src/apps/GameInfer/gui/MainWindow.cpp`（删除）
- `src/apps/GameInfer/CMakeLists.txt`

---

### T-24: 迁移 MinLabel 到 AppShell 🟡 ⛓️
**耗时**: 0.3d | **依赖**: T-16, T-22 | **执行批次**: B-8

**步骤**:
1. 修改 `src/apps/MinLabel/main.cpp`: AppShell + MinLabelPage
2. 删除旧 MainWindow
3. 验证: 菜单栏、拖拽打开文件、保存、G2P 功能

**风险说明**: MinLabel 拖拽逻辑较复杂（支持多种文件格式），需确认 AppShell 拖拽转发正确。

**产出物**: MinLabel 使用 AppShell 运行

**检验方式**: MinLabel.exe 启动；拖拽 .csv/.lab 文件正确加载

**涉及文件**:
- `src/apps/MinLabel/main.cpp`
- `src/apps/MinLabel/gui/MainWindow.h`（删除）
- `src/apps/MinLabel/gui/MainWindow.cpp`（删除）

---

### T-25: 迁移 PhonemeLabeler 到 AppShell 🟡 ⛓️
**耗时**: 0.3d | **依赖**: T-17, T-22 | **执行批次**: B-8

**步骤**:
1. 修改 `src/apps/PhonemeLabeler/main.cpp`: AppShell + PhonemeLabelerPage
2. 删除旧 MainWindow
3. 验证: 状态栏显示、快捷键、TextGrid 编辑功能

**风险说明**: 状态栏内容动态更新逻辑需确认在 AppShell 框架下仍正常。

**产出物**: PhonemeLabeler 使用 AppShell 运行

**检验方式**: PhonemeLabeler.exe 启动；打开 TextGrid 编辑正常

**涉及文件**:
- `src/apps/PhonemeLabeler/main.cpp`
- `src/apps/PhonemeLabeler/gui/MainWindow.h`（删除）
- `src/apps/PhonemeLabeler/gui/MainWindow.cpp`（删除）

---

### T-26: 迁移 PitchLabeler 到 AppShell 🟡 ⛓️
**耗时**: 0.3d | **依赖**: T-18, T-22 | **执行批次**: B-8

**步骤**:
1. 修改 `src/apps/PitchLabeler/main.cpp`: AppShell + PitchLabelerPage
2. 删除旧 MainWindow
3. 验证: openDirectory 功能、F0 编辑、A/B 比较

**风险说明**: PitchLabeler 有 openDirectory 特殊启动逻辑，需确认 AppShell 支持。

**产出物**: PitchLabeler 使用 AppShell 运行

**检验方式**: PitchLabeler.exe 启动；打开 .ds 文件编辑正常

**涉及文件**:
- `src/apps/PitchLabeler/main.cpp`
- `src/apps/PitchLabeler/gui/MainWindow.h`（删除）
- `src/apps/PitchLabeler/gui/MainWindow.cpp`（删除）

---

### T-27: 迁移 DiffSingerLabeler 到 AppShell 多页面模式 🔴 ⛓️
**耗时**: 1.5d | **依赖**: T-23~T-26 | **执行批次**: B-9

**步骤**:
1. 修改 `src/apps/labeler/main.cpp`: AppShell 多页面模式注册 9 个页面（SlicerPage, LyricFAPage, MinLabelPage, HubertFAPage, PhonemeLabelerPage, BuildCsvPage, BuildMidiPage, BuildDsPage, PitchLabelerPage）
2. 通过 `setGlobalFileActions()` 注入全局操作: Set Working Directory, Open/Save Project, Exit
3. 删除 `LabelerWindow`、`StepNavigator`；验证 9 页面切换 + 菜单/快捷键/状态栏 + 工作目录传递

**风险说明**: 这是最复杂的迁移。9 个页面的菜单栏、快捷键、状态栏各不相同，切换时必须正确替换。全局操作（工作目录）需要传递到每个页面。StepNavigator 的"下一步"逻辑需要在 AppShell 中找到替代方案（可能是 IconNavBar 的顺序导航）。任何一个页面的接口实现不完整都会在此暴露。

**产出物**: DiffSingerLabeler 使用 AppShell 多页面模式运行

**检验方式**: DiffSingerLabeler.exe 启动；9 个页面切换正常；完整走完一次标注流程

**涉及文件**:
- `src/apps/labeler/main.cpp`
- `src/apps/labeler/LabelerWindow.h`（删除）
- `src/apps/labeler/LabelerWindow.cpp`（删除）
- `src/apps/labeler/StepNavigator.h`（删除）
- `src/apps/labeler/StepNavigator.cpp`（删除）
- `src/apps/labeler/CMakeLists.txt`

---

### T-28: 清理旧代码 + 回归测试 🟡 ⛓️
**耗时**: 1d | **依赖**: T-27 | **执行批次**: B-10

**步骤**:
1. 删除所有旧 MainWindow 类（5 个应用）、StepNavigator、GameAlignPage 等遗留代码
2. 确认无残留 `buildMenuBar()`/`buildStatusBar()` 代码（grep 验证）
3. 三平台构建 + 6 个应用功能回归测试

**风险说明**: 删除代码可能遗漏某些被间接引用的类。需确认 CMakeLists 中无悬空引用。

**产出物**: 代码库干净，无遗留旧壳代码

**检验方式**: 三平台编译通过；6 个应用全部正常启动和基本操作

---

### ✅ Phase 2.5 检查点
- [ ] AppShell 组件功能完整（单页/多页模式）
- [ ] 6 个应用全部迁移到 AppShell
- [ ] 所有旧 MainWindow/LabelerWindow 已删除
- [ ] 三平台编译通过
- [ ] 功能回归: 每个应用核心功能验证通过
- [ ] 快捷键无跨页面冲突
- [ ] 拖拽功能正常

---

## Phase 3: Pipeline 子模块独立化（2 ~ 3 天）

### T-29: ✅ slicer 独立化 → slicer-lib 🟢 ⏩
**耗时**: 0.5d | **依赖**: T-03 | **执行批次**: B-3

**步骤**:
1. 新建 `src/libs/slicer/CMakeLists.txt`，声明 `slicer-lib` STATIC 库（仅依赖 Qt6::Core）
2. 搬迁纯算法文件: `Slicer.h/.cpp`, `MathUtils.h`, `Enumerations.h` 从 `src/apps/pipeline/slicer/` → `src/libs/slicer/`
3. 修改 `src/apps/pipeline/CMakeLists.txt` 链接 `slicer-lib`；更新 `src/libs/CMakeLists.txt`

**风险说明**: 纯算法分离，操作清晰。

**产出物**: `slicer-lib` 静态库可编译

**检验方式**: `cmake --build build --target slicer-lib` 通过

**涉及文件**:
- `src/libs/slicer/CMakeLists.txt`（新建）
- `src/libs/slicer/Slicer.h`, `Slicer.cpp`, `MathUtils.h`, `Enumerations.h`（搬迁）
- `src/libs/CMakeLists.txt`
- `src/apps/pipeline/CMakeLists.txt`
- `src/apps/pipeline/slicer/WorkThread.cpp`（更新 include）

---

### T-30: ✅ lyricfa 独立化 → lyricfa-lib 🟢 ⏩
**耗时**: 0.5d | **依赖**: T-03 | **执行批次**: B-3

**步骤**:
1. 新建 `src/libs/lyricfa/CMakeLists.txt`，声明 `lyricfa-lib`（依赖 `dsfw-core`, Qt6::Core）
2. 搬迁 9 对算法文件: Asr, AsrTask, ChineseProcessor, LyricData, LyricMatcher, LyricMatchTask, MatchLyric, SequenceAligner, Utils
3. UI 文件（LyricFAPage, SmartHighlighter）留在 pipeline；更新 CMake 链接

**风险说明**: 文件较多但操作机械。

**产出物**: `lyricfa-lib` 静态库可编译

**检验方式**: `cmake --build build --target lyricfa-lib` 通过

**涉及文件**:
- `src/libs/lyricfa/CMakeLists.txt`（新建）
- `src/libs/lyricfa/*.h`, `*.cpp`（9 对文件搬迁）
- `src/libs/CMakeLists.txt`
- `src/apps/pipeline/CMakeLists.txt`

---

### T-31: ✅ hubertfa 独立化 → hubertfa-lib 🟢 ⏩
**耗时**: 0.3d | **依赖**: T-03 | **执行批次**: B-3

**步骤**:
1. 新建 `src/libs/hubertfa/CMakeLists.txt`，声明 `hubertfa-lib`（依赖 `dsfw-core`, Qt6::Core）
2. 搬迁 `HfaTask.h/.cpp`；UI 文件 `HubertFAPage.h/.cpp` 留在 pipeline
3. 更新 CMake 链接

**风险说明**: 文件最少，风险最低。

**产出物**: `hubertfa-lib` 静态库可编译

**检验方式**: `cmake --build build --target hubertfa-lib` 通过

**涉及文件**:
- `src/libs/hubertfa/CMakeLists.txt`（新建）
- `src/libs/hubertfa/HfaTask.h`, `HfaTask.cpp`（搬迁）
- `src/libs/CMakeLists.txt`
- `src/apps/pipeline/CMakeLists.txt`

---

### T-32: WorkThread 业务逻辑分离 🟡 ⛓️
**耗时**: 1d | **依赖**: T-29, T-30, T-31 | **执行批次**: B-6

**步骤**:
1. 在 `slicer-lib` 中新增 `SliceJob` 类: 纯输入→输出，无 QObject/信号槽
2. `WorkThread` 改为薄壳: 持有 `SliceJob`，`run()` 中调用 `SliceJob::execute()` 后 emit 信号
3. lyricfa/hubertfa 的 Task 类确认算法不依赖 QThread；编译验证

**风险说明**: 拆分后需确保线程取消逻辑仍正常工作。SliceJob 需要支持 cancel 标志位。

**产出物**: 算法逻辑与线程管理解耦

**检验方式**: 编译通过；DatasetPipeline slicer tab 功能正常

**涉及文件**:
- `src/libs/slicer/SliceJob.h`（新建）
- `src/libs/slicer/SliceJob.cpp`（新建）
- `src/apps/pipeline/slicer/WorkThread.h`
- `src/apps/pipeline/slicer/WorkThread.cpp`

---

### T-33: Pipeline 全量验证 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-32 | **执行批次**: B-6

**步骤**:
1. 编译全部 6 个应用
2. 手动运行 `DatasetPipeline.exe`，测试 slicer/lyricfa/hubertfa 三个 tab
3. 运行已有自动化测试

**风险说明**: 低风险验证任务。

**产出物**: Pipeline 功能确认无回归

**检验方式**: 三个 tab 基本功能正常；自动化测试通过

---

### ✅ Phase 3 检查点
- [ ] `slicer-lib`、`lyricfa-lib`、`hubertfa-lib` 三个独立库可编译
- [ ] 各库仅依赖 Qt6::Core（+ dsfw-core），不依赖 Qt6::Widgets
- [ ] DatasetPipeline 三个 tab 功能正常
- [ ] WorkThread 已降为薄壳

---

## Phase 4: 框架打包为独立 CMake 包（2 ~ 3 天）

### T-34: ✅ 命名空间化所有框架 target 🟢 ⏩
**耗时**: 0.3d | **依赖**: T-03 | **执行批次**: B-3

**步骤**:
1. 在 `src/framework/core/CMakeLists.txt` 添加 `add_library(dsfw::core ALIAS dsfw-core)` + 设置导出名
2. 在 `src/framework/ui-core/CMakeLists.txt` 添加 `add_library(dsfw::ui-core ALIAS dsfw-ui-core)`
3. 编译验证别名可用

**风险说明**: 纯 CMake 元数据操作，不影响编译产物。

**产出物**: `dsfw::core` / `dsfw::ui-core` 别名可用

**检验方式**: 使用别名链接编译通过

**涉及文件**:
- `src/framework/core/CMakeLists.txt`
- `src/framework/ui-core/CMakeLists.txt`

---

### T-35: 编写 CMake export 配置 🟡 ⛓️
**耗时**: 1d | **依赖**: T-34, T-11 | **执行批次**: B-7

**步骤**:
1. 在 `src/framework/CMakeLists.txt` 添加 `install(TARGETS dsfw-core dsfw-ui-core EXPORT dsfwTargets ...)`
2. 新建 `cmake/dsfwConfig.cmake.in` 模板，使用 `@PACKAGE_INIT@` + `include("${CMAKE_CURRENT_LIST_DIR}/dsfwTargets.cmake")`
3. 生成 `dsfwConfig.cmake` + `dsfwConfigVersion.cmake`（CMakePackageConfigHelpers）

**风险说明**: CMake export 配置容易出错（路径不对、依赖传播不正确）。需仔细处理 INTERFACE_INCLUDE_DIRECTORIES 的 BUILD/INSTALL 区分。

**产出物**: `cmake --install` 后生成完整的 CMake 包

**检验方式**: `cmake --install build --prefix=./install` 生成 `lib/cmake/dsfw/dsfwConfig.cmake`

**涉及文件**:
- `src/framework/CMakeLists.txt`
- `cmake/dsfwConfig.cmake.in`（新建）

---

### T-36: 编写 install 规则 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-35 | **执行批次**: B-7

**步骤**:
1. 头文件: `install(DIRECTORY include/dsfw/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dsfw)`
2. 静态库: 已通过 `install(TARGETS ...)` 处理
3. CMake 配置: `install(EXPORT dsfwTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dsfw NAMESPACE dsfw::)`

**风险说明**: 安装路径在不同平台有差异（lib vs lib64）。需使用 GNUInstallDirs。

**产出物**: `cmake --install` 产生完整的可分发框架包

**检验方式**: install 目录结构正确（include/dsfw/*.h + lib/*.a + lib/cmake/dsfw/*.cmake）

**涉及文件**:
- `src/framework/core/CMakeLists.txt`
- `src/framework/ui-core/CMakeLists.txt`
- `src/framework/CMakeLists.txt`

---

### T-37: 验证 find_package 可用 🟢 ⛓️
**耗时**: 0.5d | **依赖**: T-36 | **执行批次**: B-8

**步骤**:
1. 新建 `tests/test-find-package/CMakeLists.txt` + `main.cpp`（最小示例，`find_package(dsfw REQUIRED)` + `target_link_libraries(... dsfw::core)`）
2. 配置: `cmake -B build -DCMAKE_PREFIX_PATH=<install-dir>`
3. 编译通过即验证成功

**风险说明**: 若 export 配置有误会在此暴露。

**产出物**: 外部项目可通过 `find_package(dsfw)` 使用框架

**检验方式**: test-find-package 项目编译链接通过

**涉及文件**:
- `tests/test-find-package/CMakeLists.txt`（新建）
- `tests/test-find-package/main.cpp`（新建）

---

### T-38: 更新应用层使用 find_package 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-37 | **执行批次**: B-8

**步骤**:
1. （可选）在 `src/domain/CMakeLists.txt` 中改用 `find_package(dsfw)` + `dsfw::core`
2. 同一仓库内保持 subdirectory 模式；find_package 仅验证外部可用性
3. 在 CI 中加入 find_package 验证步骤

**风险说明**: 同仓库内不强制 find_package，风险低。

**产出物**: CI 验证 find_package 流程

**检验方式**: CI 新增 step 通过

**涉及文件**:
- `.github/workflows/`（CI 配置）

---

### ✅ Phase 4 检查点
- [ ] `cmake --install` 产生完整框架包
- [ ] 外部项目 `find_package(dsfw)` 成功
- [ ] 命名空间别名 `dsfw::core` / `dsfw::ui-core` 可用
- [ ] CI 包含 find_package 验证

---

## Phase 5: 单元测试（3 ~ 5 天）

### T-39: 测试基础设施搭建 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-11 | **执行批次**: B-9

**步骤**:
1. 新建 `src/tests/framework/CMakeLists.txt`，选用 Qt Test 框架
2. 声明测试 target，链接 `dsfw-core` + `Qt6::Test`
3. 修改 `src/tests/CMakeLists.txt` 添加 `add_subdirectory(framework)`

**风险说明**: 简单基础设施搭建。

**产出物**: 测试 target 可编译（暂无测试用例）

**检验方式**: `cmake --build build --target dsfw-tests` 通过

**涉及文件**:
- `src/tests/framework/CMakeLists.txt`（新建）
- `src/tests/CMakeLists.txt`

---

### T-40: 框架核心类单元测试 🟡 ⛓️
**耗时**: 2.5d | **依赖**: T-39 | **执行批次**: B-9

**步骤**:
1. 编写 8 个测试文件: `test_AppSettings.cpp`（读写/默认值）、`test_JsonHelper.cpp`（序列化边界）、`test_ServiceLocator.cpp`（注册/获取/线程安全）、`test_AsyncTask.cpp`（调度/取消/进度）、`test_LocalFileIOProvider.cpp`（读写/错误处理）、`test_IDocument.cpp`（生命周期）、`test_IExportFormat.cpp`（Mock 导出）、`test_IModelProvider.cpp`（Mock 注册查询）
2. 每个测试文件使用 Qt Test 的 `QTEST_MAIN` 或统一 test runner
3. 确保 `ctest` 可发现并运行所有测试

**风险说明**: 测试编写可能暴露框架 API 设计缺陷（如 ServiceLocator 线程安全性不足），需要在此阶段修复。

**产出物**: 8 个测试文件，覆盖框架核心类

**检验方式**: `ctest --test-dir build -R dsfw` 全部 PASS

**涉及文件**:
- `src/tests/framework/test_AppSettings.cpp`（新建）
- `src/tests/framework/test_JsonHelper.cpp`（新建）
- `src/tests/framework/test_ServiceLocator.cpp`（新建）
- `src/tests/framework/test_AsyncTask.cpp`（新建）
- `src/tests/framework/test_LocalFileIOProvider.cpp`（新建）
- `src/tests/framework/test_IDocument.cpp`（新建）
- `src/tests/framework/test_IExportFormat.cpp`（新建）
- `src/tests/framework/test_IModelProvider.cpp`（新建）

---

### T-41: 集成测试模板 🟢 ⛓️
**耗时**: 0.5d | **依赖**: T-40 | **执行批次**: B-9

**步骤**:
1. 新建 `src/tests/framework/integration/test_load_process_export.cpp`: 模拟「加载文件 → 处理 → 导出」完整流程
2. 使用 Mock IDocument + Mock IExportFormat 验证端到端流程
3. 提供 `src/tests/framework/integration/README.md` 说明如何编写新集成测试

**风险说明**: 低风险，模板性质。

**产出物**: 集成测试模板 + 文档

**检验方式**: 集成测试 PASS

**涉及文件**:
- `src/tests/framework/integration/test_load_process_export.cpp`（新建）
- `src/tests/framework/integration/README.md`（新建）
- `src/tests/framework/CMakeLists.txt`（更新）

---

### T-42: CI 集成测试 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-41 | **执行批次**: B-9

**步骤**:
1. 确保 `BUILD_TESTS=ON` 编译全部新测试
2. CI workflow 添加 `ctest --test-dir build --output-on-failure`
3. 设置测试失败 → CI 标红

**风险说明**: CI 配置操作，风险低。

**产出物**: CI 自动运行框架测试

**检验方式**: CI 绿灯；故意制造失败验证标红

**涉及文件**:
- `.github/workflows/`（CI 配置）

---

### ✅ Phase 5 检查点
- [ ] 8 个核心类测试全部 PASS
- [ ] 集成测试 PASS
- [ ] CI 自动运行测试并正确报告结果
- [ ] 测试覆盖率 > 70%（核心类）

---

## Phase 6: 文档（2 ~ 3 天）

### T-43: ✅ Doxygen 配置 🟢 ⏩
**耗时**: 0.3d | **依赖**: 无（随时可做） | **执行批次**: B-3

**步骤**:
1. 在项目根目录新建 `Doxyfile`，输入路径 `src/framework/core/include/` + `src/framework/ui-core/include/`
2. 输出到 `docs/api/`（.gitignore 忽略生成产物）
3. 顶层 `CMakeLists.txt` 添加 `add_custom_target(docs COMMAND doxygen ${CMAKE_SOURCE_DIR}/Doxyfile)`

**风险说明**: 无实质风险。

**产出物**: `Doxyfile` + `docs` CMake target

**检验方式**: `cmake --build build --target docs` 生成 HTML 文档

**涉及文件**:
- `Doxyfile`（新建）
- `CMakeLists.txt`（顶层）

---

### T-44: 框架头文件 Doxygen 注释 🟢 ⛓️
**耗时**: 1d | **依赖**: T-11 | **执行批次**: B-9

**步骤**:
1. 为 12 个框架头文件补充 `///` 或 `/** */` 格式注释: 类说明、方法参数/返回值、使用示例
2. 重点: `ServiceLocator.h`（使用示例）、`IDocument.h`（接口契约）、`AppSettings.h`（SettingsKey 模板说明）
3. 运行 Doxygen 确认无 warning

**风险说明**: 文档编写，无代码风险。

**产出物**: 12 个头文件完整 Doxygen 注释

**检验方式**: Doxygen 生成无 warning；HTML 文档可浏览

**涉及文件**:
- `src/framework/core/include/dsfw/*.h`（全部 12 个）

---

### T-45: 编写框架使用指南 🟢 ⛓️
**耗时**: 1d | **依赖**: T-38 | **执行批次**: B-10

**步骤**:
1. 编写 `docs/framework-getting-started.md`: 从零基于 dsfw 构建最小应用的教程
2. 编写 `docs/framework-architecture.md`: 分层图、模块职责、依赖关系
3. 编写 `docs/migration-guide.md`: 从旧 dstools-core 迁移步骤

**风险说明**: 无代码风险。

**产出物**: 3 份框架文档

**检验方式**: 文档内容完整且示例代码可编译

**涉及文件**:
- `docs/framework-getting-started.md`（新建）
- `docs/framework-architecture.md`（新建）
- `docs/migration-guide.md`（新建）

---

### T-46: 更新项目 README 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-45 | **执行批次**: B-10

**步骤**:
1. 在 `README.md` 增加「框架」章节: dsfw 定位和用法
2. 更新「Build from Source」: 补充框架独立构建方式
3. 更新「Build Outputs」表格: 列出 `dsfw-core`、`dsfw-ui-core` 新增库

**风险说明**: 无风险。

**产出物**: README 更新

**检验方式**: 内容准确反映当前项目结构

**涉及文件**:
- `README.md`

---

### ✅ Phase 6 检查点
- [ ] Doxygen 生成完整 API 文档
- [ ] 12 个头文件注释完备
- [ ] 3 份使用指南编写完成
- [ ] README 更新

---

## 附录 A: 任务依赖矩阵

| 任务 | 依赖 | 被依赖于 |
|------|------|----------|
| T-01 | — | T-02 |
| T-02 | T-01 | T-03 |
| T-03 | T-02 | T-04, T-29, T-30, T-31, T-34, T-43 |
| T-04 | T-03 | T-05 |
| T-05 | T-04 | T-06, T-07 |
| T-06 | T-05 | T-08, T-09 |
| T-07 | T-05 | T-09 |
| T-08 | T-06 | T-09 |
| T-09 | T-06, T-07, T-08 | T-10 |
| T-10 | T-09 | T-11 |
| T-11 | T-10 | T-12, T-35, T-39, T-44 |
| T-12 | T-11 | T-13 |
| T-13 | T-12 | T-14 |
| T-14 | T-13 | — |
| T-15 | — | T-16~T-21 |
| T-16 | T-15 | T-24 |
| T-17 | T-15 | T-25 |
| T-18 | T-15 | T-26 |
| T-19 | T-15 | T-23 |
| T-20 | T-15 | — |
| T-21 | T-15 | T-22 |
| T-22 | T-15, T-21 | T-23~T-26 |
| T-23 | T-19, T-22 | T-27 |
| T-24 | T-16, T-22 | T-27 |
| T-25 | T-17, T-22 | T-27 |
| T-26 | T-18, T-22 | T-27 |
| T-27 | T-23~T-26 | T-28 |
| T-28 | T-27 | — |
| T-29 | T-03 | T-32 |
| T-30 | T-03 | T-32 |
| T-31 | T-03 | T-32 |
| T-32 | T-29, T-30, T-31 | T-33 |
| T-33 | T-32 | — |
| T-34 | T-03 | T-35 |
| T-35 | T-34, T-11 | T-36 |
| T-36 | T-35 | T-37 |
| T-37 | T-36 | T-38 |
| T-38 | T-37 | T-45 |
| T-39 | T-11 | T-40 |
| T-40 | T-39 | T-41 |
| T-41 | T-40 | T-42 |
| T-42 | T-41 | — |
| T-43 | — | T-44 |
| T-44 | T-11 | — |
| T-45 | T-38 | T-46 |
| T-46 | T-45 | — |

---

## 附录 B: 风险登记册

### 🔴 高风险任务

| 任务 | 风险描述 | 缓解措施 |
|------|----------|----------|
| T-05g (ModelManager 拆分) | 通用/领域逻辑边界模糊，拆分不当导致循环依赖 | 先画依赖图；先定接口再实现；每步编译验证 |
| T-11 (全量编译验证) | Phase 1 所有问题集中暴露：循环依赖、MOC 遗漏、链接顺序 | 预留 1 天调试时间；准备回退到 T-10 之前的方案 |
| T-22 (AppShell 实现) | 窗口框架层影响所有应用视觉交互；跨平台 FramelessHelper 行为不一致 | 先实现最小可用版本；在三平台逐步验证；保留旧 MainWindow 直到 AppShell 验证通过 |
| T-27 (DSLabeler 多页面迁移) | 9 页面菜单/快捷键/状态栏切换极复杂；全局操作注入；工作目录传递 | 逐页面迁移验证；保留 StepNavigator 备用；完整走通标注流程 |

### 🟡 中风险任务

| 任务 | 风险描述 | 缓解措施 |
|------|----------|----------|
| T-03 | CMake 骨架设计错误导致全部返工 | 参考成熟开源项目结构；团队 review |
| T-05d | ServiceLocator 遗留 API 耦合 | 先标记 deprecated 再逐步移除 |
| T-05e | IDocument DocumentFormat 枚举含领域值 | 改为字符串标识或注册机制 |
| T-05f | IModelProvider ModelType 枚举领域耦合 | 同上 |
| T-05h | ModelDownloader URL 路由边界不清 | 定义清晰回调接口 |
| T-07 | CommonKeys 分界线不清晰 | 逐一审查使用者 |
| T-09 | include 替换遗漏 | 全量编译 + grep 验证 |
| T-10 | 删除核心库遗漏依赖者 | 先降级为薄壳再删除 |
| T-12 | 初始化顺序被打乱 | 确保 hook 注册在 main() 早期 |
| T-15 | 接口变更影响所有实现 | 提供默认实现 |
| T-16~T-19 | QAction parent 迁移内存管理 | 使用 QObject parent 机制 |
| T-21 | QShortcut context 在 QStackedWidget 中行为 | 手动管理 + 测试验证 |
| T-23~T-26 | 首批迁移验证 AppShell 可行性 | 从最简单的应用开始 |
| T-28 | 删除代码遗漏间接引用 | grep + CMake 验证 |
| T-32 | 线程取消逻辑迁移 | 支持 cancel 标志位 |
| T-35 | CMake export 路径配置错误 | 使用标准 CMakePackageConfigHelpers |
| T-36 | 安装路径跨平台差异 | 使用 GNUInstallDirs |
| T-40 | 测试暴露 API 设计缺陷 | 在测试阶段修复而非忽略 |

---

## 附录 C: 检查点清单

| 阶段 | 检查项 | 验证方式 |
|------|--------|----------|
| Phase 0 完成 | 分支+CI+空壳库 | CI 绿灯 |
| Phase 1 完成 | 框架/领域分离+三平台编译+测试无回归 | `ctest` 全 PASS |
| Phase 2 完成 | UI-Core 无领域依赖+应用启动正常 | 手动启动 MinLabel/PhonemeLabeler |
| Phase 2.5 完成 | 6 应用全部使用 AppShell+功能回归 | 每个应用核心流程走通 |
| Phase 3 完成 | 3 个独立库+Pipeline 功能正常 | DatasetPipeline 三 tab 测试 |
| Phase 4 完成 | find_package 可用+CI 验证 | test-find-package 编译通过 |
| Phase 5 完成 | 单元测试+集成测试+CI | `ctest` 全 PASS |
| Phase 6 完成 | 文档齐全+README 更新 | Doxygen 无 warning |
| **最终验收** | 全部上述 + 6 应用完整功能测试 | 三平台 CI 全绿 + 手动回归 |
