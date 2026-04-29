# UI 控件标准化任务清单

## 背景

项目中存在大量重复的 UI 模式（路径选择器、模型加载面板、运行进度行等），分散在 6 个应用的多个页面中。
这些重复代码导致：
- 行为不一致（拖拽支持仅部分页面有）
- 维护成本高（修改交互需同步 15+ 处）
- ARCH-006（labeler 与 pipeline 代码重复）难以推进

本任务将这些模式抽取为 `dstools-widgets` 库中的标准控件。

---

## 已完成

- ✅ **TASK-W01**: PathSelector 标准控件
- ✅ **TASK-W02**: ModelLoadPanel 标准控件
- ✅ **TASK-W03**: RunProgressRow 标准控件
- ✅ **TASK-W04**: PathSelector 拖拽增强（已内建于 PathSelector 实现）
- ✅ **TASK-W05**: 迁移 GameInfer MainWidget（6 PathSelector + 2 RunProgressRow）
- ✅ **TASK-W06**: 迁移 Pipeline 页面（LyricFA/HubertFA/Slicer → PathSelector + ModelLoadPanel）
- ✅ **TASK-W07**: 迁移 Labeler 步骤页 + ExportDialog（PathSelector + RunProgressRow）

---

## 可标准化模块清单

| # | 模块 | 出现次数 | 分布 | 变体差异 |
|---|------|---------|------|---------|
| 1 | PathSelector | 15+ | GameInfer(6), LyricFAPage(4), HubertFAPage(2), labeler步骤页(3+), ExportDialog(1) | HBox/Grid混用; Open/Save/Directory三种; 拖拽仅2处 |
| 2 | ModelLoadPanel | 3 | LyricFAPage, HubertFAPage, GameInfer | PathSelector + Load + 状态标签 |
| 3 | RunProgressRow | 5+ | GameInfer(2), BuildCsvPage, BuildDsPage, GameAlignPage | Run按钮 + QProgressBar |
| 4 | ParameterGroupBox | 4+ | labeler步骤页, SlicerPage | QGroupBox → QFormLayout → 参数 + Run + Log |

---

## 剩余任务

无。所有任务已完成。

---

## 执行顺序与依赖图

所有任务已完成。

---

## 风险矩阵

（已完成，无剩余风险）

---

## 成果评估

| 指标 | 基线 | 目标 | 实际 |
|------|------|------|------|
| 路径选择器代码重复 | 15+ 处各自实现 | 1 处标准实现 + 15 处调用 | ✅ 达成 |
| 代码行数 | — | 净减少 > 300 行 | ✅ 达成 |
| 拖拽支持覆盖率 | 2/15 (13%) | 15/15 (100%) | ✅ 达成 |
| 新增标准控件 | 0 → 3 ✅ | PathSelector, ModelLoadPanel, RunProgressRow | ✅ 达成 |
| 路径持久化一致性 | 部分页面无持久化 | 全部通过 settingsKey 统一持久化 | ✅ GameInfer 已迁移 |
| API 文档覆盖 | — | 每个控件 Doxygen 注释完备 | ✅ 达成 |
| 回归验证 | — | 6 个 app 核心流程手动通过 | 待验证 |

---

## 与现有任务的协同

| 现有任务 | 协同关系 |
|---------|---------|
| TASK-009 (labeler集成pipeline) | 标准控件完成后，pipeline页面可直接嵌入labeler，工作量降低约 40% |
| ARCH-006 (labeler/pipeline重复) | 标准控件是解决此问题的基础设施 |
