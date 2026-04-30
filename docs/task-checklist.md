# 框架抽取实施路线图

> **关键路径**: T-09 → T-10 → T-11 → T-22 → T-27 → T-28  
> **当前进度**: Phase 0 ✅ | Phase 1 进行中（T-05a~f ✅，T-06~T-08 ✅，剩余 T-05g/h, T-09~T-11）| Phase 2 ✅ | Phase 2.5 进行中 | Phase 3 部分就绪  

---

## 术语和图例

| 图标 | 含义 |
|------|------|
| 🟢 | 低风险 — 操作机械、影响面小 |
| 🟡 | 中风险 — 需要判断或影响面较大 |
| 🔴 | 高风险 — 可能导致大面积返工或阻塞 |
| ⏩ | 可与同批次其他任务并行 |
| ⛓️ | 串行，必须等前置任务完成 |

---

## Phase 1: Core 层分离

### T-05g: ModelManager 拆分 🔴
- 提取通用 `IModelRegistry` 接口到 `dsfw/`
- DiffSinger 特定逻辑留在 `src/domain/`
- **风险**: 混合通用/领域逻辑，拆分边界需仔细判断

### T-05h: ModelDownloader 🟡
- 通用下载逻辑搬到 `dsfw/`，模型 URL 配置留在 `src/domain/`

**检验方式**: `cmake --build build --target dsfw-core` 通过；`dsfw-core` 不依赖 `dstools` 命名空间

---

### T-09: 更新所有 include 路径 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-06 ✅, T-07 ✅ | **执行批次**: B-6

- 全局搜索替换: `#include <dstools/XXX.h>` → `#include <dsfw/XXX.h>`（12 个框架类）
- 影响 `src/apps/`、`src/ui-core/`、`src/widgets/`、`src/domain/`

**检验方式**: `cmake --build build` 全量通过

---

### T-10: 删除/降级旧 dstools-core 🟡 ⛓️
**耗时**: 0.3d | **依赖**: T-09 | **执行批次**: B-7

- 降级为薄壳或直接删除 `src/core/`
- 更新所有 `target_link_libraries` 从 `dstools-core` 改为 `dsfw-core` + `dstools-domain`

---

### T-11: 全量编译验证 🔴 ⛓️
**耗时**: 0.5d | **依赖**: T-10 | **执行批次**: B-7

- 三平台全量编译
- 运行已有测试 `TestAudioUtil`、`TestGame`、`TestRmvpe`

**检验方式**: CI 三平台全绿；`ctest` 全部 PASS

---

### Phase 1 检查点
- [ ] `dsfw-core` 包含全部 12+ 通用类，独立编译通过
- [ ] `dstools-domain` 包含全部 14 领域类，编译通过
- [ ] 旧 `src/core/` 已删除或降级
- [ ] 三平台全量编译通过
- [ ] 已有测试无回归

---

## Phase 2: UI-Core 清理

### T-14: 验证 CommonKeys 在 UI 层的引用 🟢 ⛓️
**耗时**: 0.2d | **依赖**: T-13 ✅ | **执行批次**: B-8

---

## Phase 2.5: AppShell 实现与迁移

### T-16: MinLabelPage 实现完整接口 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-15 ✅ | **执行批次**: B-5

- 从 MainWindow 提取菜单/拖拽/状态栏逻辑到 MinLabelPage

---

### T-17: PhonemeLabelerPage 实现完整接口 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-15 ✅ | **执行批次**: B-5

---

### T-18: PitchLabelerPage 实现完整接口 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-15 ✅ | **执行批次**: B-5

---

### T-20: Pipeline 页面适配 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-15 ✅ | **执行批次**: B-5

- 补充 TaskWindowAdapter / BuildCsvPage / BuildDsPage 的 `createMenuBar()`

---

### T-22: 实现 AppShell 框架组件 🔴 ⛓️
**耗时**: 2d | **依赖**: T-15 ✅, T-21 ✅ | **执行批次**: B-6

- AppShell(QMainWindow): FramelessHelper + TitleBar + IconNavBar + QStackedWidget + QStatusBar
- 核心 API: `addPage()`, 页面切换（菜单/快捷键/标题/状态栏）, 单页模式, 全局操作注入, closeEvent, 拖拽转发

**风险**: 窗口框架层变更影响所有应用；跨平台 FramelessHelper 行为不一致

---

### T-23: 迁移 GameInfer 到 AppShell 🟡 ⛓️
**耗时**: 0.3d | **依赖**: T-19 ✅, T-22 | **执行批次**: B-8

---

### T-24: 迁移 MinLabel 到 AppShell 🟡 ⛓️
**耗时**: 0.3d | **依赖**: T-16, T-22 | **执行批次**: B-8

---

### T-25: 迁移 PhonemeLabeler 到 AppShell 🟡 ⛓️
**耗时**: 0.3d | **依赖**: T-17, T-22 | **执行批次**: B-8

---

### T-26: 迁移 PitchLabeler 到 AppShell 🟡 ⛓️
**耗时**: 0.3d | **依赖**: T-18, T-22 | **执行批次**: B-8

---

### T-27: 迁移 DiffSingerLabeler 到 AppShell 多页面模式 🔴 ⛓️
**耗时**: 1.5d | **依赖**: T-23~T-26 | **执行批次**: B-9

- 9 页面注册、全局操作注入、删除 LabelerWindow/StepNavigator

**风险**: 最复杂的迁移，菜单/快捷键/状态栏切换 + 全局操作 + 工作目录传递

---

### T-28: 清理旧代码 + 回归测试 🟡 ⛓️
**耗时**: 1d | **依赖**: T-27 | **执行批次**: B-10

- 删除所有旧 MainWindow、StepNavigator 等遗留代码
- 三平台构建 + 6 个应用功能回归

---

### Phase 2.5 检查点
- [ ] AppShell 组件功能完整（单页/多页模式）
- [ ] 6 个应用全部迁移到 AppShell
- [ ] 所有旧 MainWindow/LabelerWindow 已删除
- [ ] 三平台编译通过 + 功能回归通过

---

## Phase 3: Pipeline 子模块独立化

### T-32: WorkThread 业务逻辑分离 🟡 ⛓️
**耗时**: 1d | **依赖**: T-29~T-31 ✅ | **执行批次**: B-6

- 在 `slicer-lib` 中新增 `SliceJob` 纯算法类
- `WorkThread` 改为薄壳

---

### T-33: Pipeline 全量验证 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-32 | **执行批次**: B-6

- 编译全部应用，手动测试 slicer/lyricfa/hubertfa

---

### Phase 3 检查点
- [ ] WorkThread 已降为薄壳
- [ ] DatasetPipeline 三个 tab 功能正常

---

## Phase 4: 框架打包为独立 CMake 包

### T-35: 编写 CMake export 配置 🟡 ⛓️
**耗时**: 1d | **依赖**: T-34 ✅, T-11 | **执行批次**: B-7

- `install(TARGETS ... EXPORT dsfwTargets ...)`
- 新建 `cmake/dsfwConfig.cmake.in`

---

### T-36: 编写 install 规则 🟡 ⛓️
**耗时**: 0.5d | **依赖**: T-35 | **执行批次**: B-7

---

### T-37: 验证 find_package 可用 🟢 ⛓️
**耗时**: 0.5d | **依赖**: T-36 | **执行批次**: B-8

- 新建 `tests/test-find-package/` 最小示例验证

---

### T-38: 更新应用层使用 find_package 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-37 | **执行批次**: B-8

- CI 中加入 find_package 验证步骤

---

### Phase 4 检查点
- [ ] `cmake --install` 产生完整框架包
- [ ] 外部项目 `find_package(dsfw)` 成功
- [ ] CI 包含 find_package 验证

---

## Phase 5: 单元测试

### T-39: 测试基础设施搭建 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-11 | **执行批次**: B-9

---

### T-40: 框架核心类单元测试 🟡 ⛓️
**耗时**: 2.5d | **依赖**: T-39 | **执行批次**: B-9

- 编写 8 个测试文件覆盖框架核心类

---

### T-41: 集成测试模板 🟢 ⛓️
**耗时**: 0.5d | **依赖**: T-40 | **执行批次**: B-9

---

### T-42: CI 集成测试 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-41 | **执行批次**: B-9

---

### Phase 5 检查点
- [ ] 8 个核心类测试全部 PASS
- [ ] CI 自动运行测试

---

## Phase 6: 文档

### T-44: 框架头文件 Doxygen 注释 🟢 ⛓️
**耗时**: 1d | **依赖**: T-11 | **执行批次**: B-9

---

### T-45: 编写框架使用指南 🟢 ⛓️
**耗时**: 1d | **依赖**: T-38 | **执行批次**: B-10

- `docs/framework-getting-started.md`、`docs/framework-architecture.md`、`docs/migration-guide.md`

---

### T-46: 更新项目 README 🟢 ⛓️
**耗时**: 0.3d | **依赖**: T-45 | **执行批次**: B-10

---

### Phase 6 检查点
- [ ] Doxygen 注释完备
- [ ] 3 份使用指南完成
- [ ] README 更新

---

## 附录: 高风险任务速查

| 任务 | 风险描述 | 缓解措施 |
|------|----------|----------|
| T-05g (ModelManager 拆分) | 通用/领域逻辑边界模糊，拆分不当导致循环依赖 | 先画依赖图；先定接口再实现 |
| T-11 (全量编译验证) | Phase 1 所有问题集中暴露 | 预留 1 天调试时间 |
| T-22 (AppShell 实现) | 窗口框架层影响所有应用；跨平台行为不一致 | 先最小可用版本；三平台逐步验证 |
| T-27 (DSLabeler 多页面迁移) | 9 页面切换极复杂 | 逐页面迁移验证；完整走通标注流程 |

---

## 下一步行动

**立即可开始（无阻塞依赖）**:
- T-09（更新所有 include 路径）— 依赖 T-06 ✅, T-07 ✅
- T-05g/h（ModelManager 拆分 + ModelDownloader 迁移）
- T-14（验证 CommonKeys 在 UI 层的引用，依赖 T-13 ✅）
- T-16 ~ T-18, T-20（Page 接口实现，依赖 T-15 ✅）
- T-32（WorkThread 分离，依赖 T-29~T-31 ✅）
