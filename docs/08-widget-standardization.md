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

### TASK-W04: PathSelector 拖拽增强 [P2]

**目标**: 统一拖拽行为，支持 Mode 校验

**实现**:
1. Directory 模式: 只接受拖入的目录路径
2. OpenFile 模式: 校验后缀是否匹配 filter
3. 多文件拖入: 取第一个有效路径
4. 无效拖入: 光标显示禁止图标

**依赖**: TASK-W01  
**工作量**: 0.5 人天  
**风险**: 低  
**评估标准**:
- [ ] 拖入不匹配类型时拒绝
- [ ] 拖入匹配文件时正确设置 path
- [ ] Directory 模式拖入文件时自动取其父目录（或拒绝）

---

### TASK-W05: 迁移 GameInfer MainWidget [P2]

**目标**: 替换 6 个路径选择器 + 2 个 Run+Progress

**涉及文件**: `src/apps/GameInfer/gui/MainWidget.cpp` (L90, L264, L279, L632, L640, L648)

**迁移步骤**:
1. 替换 Grid 布局中的 QLabel+QLineEdit+QPushButton 为 PathSelector
2. 替换 Run+QProgressBar 为 RunProgressRow
3. 连接信号到现有业务逻辑
4. 删除旧 UI 创建代码

**工作量**: 1.5 人天  
**风险**: 中 — GameInfer 有已知线程安全问题  
**评估标准**:
- [ ] 编译通过
- [ ] MIDI 转写全流程正常（CPU + GPU）
- [ ] 路径持久化行为不变
- [ ] 净删除 > 100 行

---

### TASK-W06: 迁移 Pipeline 页面 [P2]

**目标**: LyricFAPage(4 路径 + 1 模型面板), HubertFAPage(2 路径 + 1 模型面板), SlicerPage

**涉及文件**:
- `src/apps/pipeline/lyricfa/LyricFAPage.cpp`
- `src/apps/pipeline/hubertfa/HubertFAPage.cpp`
- `src/apps/pipeline/slicer/SlicerPage.cpp`

**迁移步骤**:
1. PathSelector 替换手动布局
2. ModelLoadPanel 替换模型加载区域
3. 验证 TaskWindow 基类交互不受影响

**工作量**: 1.5 人天  
**风险**: 低  
**评估标准**:
- [ ] 编译通过
- [ ] Slicer/LyricFA/HubertFA 全流程正常
- [ ] 模型加载/状态显示正常

---

### TASK-W07: 迁移 Labeler 步骤页 + ExportDialog [P3]

**目标**: BuildCsvPage, BuildDsPage, GameAlignPage, ExportDialog

**涉及文件**:
- `src/apps/labeler/pages/BuildCsvPage.cpp`
- `src/apps/labeler/pages/BuildDsPage.cpp`
- `src/apps/labeler/pages/GameAlignPage.cpp`
- `src/apps/MinLabel/gui/ExportDialog.cpp`

**工作量**: 1 人天  
**风险**: 低 — 多数为 stub 页面  
**评估标准**:
- [ ] 编译通过
- [ ] ExportDialog 导出路径选择正常

---

## 执行顺序与依赖图

```
TASK-W04 (拖拽增强) ─→ TASK-W05~W07 (迁移)

TASK-W05~W07 (迁移) 可使用已完成的 PathSelector, ModelLoadPanel, RunProgressRow
```

**推荐执行路径**:
```
Phase 1: W04 (0.5天)
Phase 2: W06 → W05 → W07 (迁移, 4天)
总计: ~4.5 人天
```

---

## 风险矩阵

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|---------|
| PathSelector API 不满足某实例需求 | 低 | 中 | 已实现，可按需扩展 |
| 迁移后行为细微差异 | 低 | 中 | 每次迁移后手动回归对应 app |
| widgets DLL 体积/依赖膨胀 | 极低 | 低 | 新控件 < 500 行，无新外部依赖 |

---

## 成果评估

| 指标 | 基线 | 目标 |
|------|------|------|
| 路径选择器代码重复 | 15+ 处各自实现 | 1 处标准实现 + 15 处调用 |
| 代码行数 | — | 净减少 > 300 行 |
| 拖拽支持覆盖率 | 2/15 (13%) | 15/15 (100%) |
| 新增标准控件 | 0 → 3 ✅ | PathSelector, ModelLoadPanel, RunProgressRow |
| 路径持久化一致性 | 部分页面无持久化 | 全部通过 settingsKey 统一持久化 |
| API 文档覆盖 | — | 每个控件 Doxygen 注释完备 |
| 回归验证 | — | 6 个 app 核心流程手动通过 |

---

## 与现有任务的协同

| 现有任务 | 协同关系 |
|---------|---------|
| TASK-009 (labeler集成pipeline) | 标准控件完成后，pipeline页面可直接嵌入labeler，工作量降低约 40% |
| ARCH-006 (labeler/pipeline重复) | 标准控件是解决此问题的基础设施 |
