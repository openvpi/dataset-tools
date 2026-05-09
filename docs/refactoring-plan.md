# 重构方案

> 2026-05-09 — 新增 T-16~T-24 重构任务
>
> 设计准则与决策详见 `human-decisions.md`

---

## 关键架构决策 (ADR)

| ADR | 决策 | 状态 |
|-----|------|------|
| 8 | 单仓库模式 | 有效 |
| 43 | int64 微秒时间精度 | 有效 |
| 46 | LabelSuite + DsLabeler 两个 exe | 有效 |
| 52 | IEditorDataSource 抽象数据源 | 有效 |
| 57 | 层依赖 DAG + per-slice dirty 标记 | 有效 |
| 78 | Slicer/Phoneme 统一视图组合（D-30） | 有效 |
| 79 | Log 侧边栏改为独立 LogPage | 有效 |
| 80 | PlayWidget 禁止 ServiceLocator 共享 IAudioPlayer（D-31） | 有效 |
| 81 | invalidateModel 先发信号再卸载（D-32） | 有效 |
| 85 | applyFaResult 直接替换 grapheme 层（修订 D-42） | **已完成** |
| 89 | 激活层贯穿线显示修复（D-40） | **已完成** |
| 90 | 贯穿线拖动修复——chart 绘制边界线（D-41） | **已完成** |
| 91 | FA 原生输出层级从属关系（D-42） | **已完成** |
| 92 | PitchLabeler 工具栏按钮 + 音频播放（D-43） | **已完成** |
| 93 | FA grapheme 层 SP 缺失/重复层级修复（BUG-24/25） | **已完成** |
| 94 | PitchLabeler 音频播放修复（BUG-26） | **已完成** |
| 95 | 移除 loadFromDsText 中 CJK 重命名逻辑（BUG-32） | **已完成** |
| 96 | QtConcurrent::run lambda 推理调用添加 try-catch（BUG-27/TD-22） | **已完成** |
| 97 | 统一路径库 PathUtils 增强（P-10） | **已完成** |
| 98 | 废弃 dstools::toFsPath / DsDocument::toFsPath（P-10） | **已完成** |
| 99 | JsonHelper 内部 pathToUtf8 改用 PathUtils::toUtf8（P-10） | **已完成** |
| 116 | fa_grapheme 层改名为 grapheme（D-44） | **待执行** |
| 117 | HFA 结果使用 fill_small_gaps + add_SP 填充微小空隙（D-45） | **待执行** |
| 118 | 刻度条系统绑定 ViewportController resolution（D-46） | **待执行** |
| 119 | PitchLabel 缩放限制最大长度为音频时长（D-47） | **待执行** |
| 120 | 修复音高线 F0Curve timestep 单位转换导致不显示（D-48/BUG-33） | **待执行** |
| 121 | PitchLabel MIDI 优先 align 模式，无 ph_dur 时弹窗降级（D-49） | **待执行** |
| 122 | 各步骤特殊关键字在设置页面体现（D-50） | **待执行** |
| 123 | 分析修复各应用控件实时刷新问题（D-51） | **待执行** |
| 124 | 整理归纳文档，删去过时内容，合理分类归位（D-52） | **待执行** |

---

## 待办任务

| 任务 | 关联 | 优先级 | 状态 |
|------|------|--------|------|
| T-16: fa_grapheme 改名为 grapheme | ADR-116, D-44 | 高 | ⬜ 待执行 |
| T-17: HFA fill_small_gaps + add_SP 填充微小空隙 | ADR-117, D-45 | 高 | ⬜ 待执行 |
| T-18: 重新设计刻度条系统——绑定 resolution | ADR-118, D-46 | 高 | ⬜ 待执行 |
| T-19: PitchLabel 缩放限制最大长度 | ADR-119, D-47 | 高 | ⬜ 待执行 |
| T-20: 修复音高线 F0Curve timestep 单位转换 bug | ADR-120, D-48, BUG-33 | 高 | ⬜ 待执行 |
| T-21: PitchLabel MIDI align 模式 + ph_dur 弹窗 | ADR-121, D-49 | 高 | ⬜ 待执行 |
| T-22: 各步骤特殊关键字在设置页面体现 | ADR-122, D-50 | 中 | ⬜ 待执行 |
| T-23: 分析修复各应用控件实时刷新 | ADR-123, D-51 | 中 | ⬜ 待执行 |
| T-24: 整理归纳文档 | ADR-124, D-52 | 中 | ⬜ 待执行 |

### 已完成任务

> 以下任务已在前序迭代中完成，保留仅作历史记录。

| 任务 | 关联 | 提交 |
|------|------|------|
| T-01: fa_grapheme 层代替直接替换 grapheme（UC-01） | ADR-100, D-42 回滚 | `by-git` |
| T-02: 贯穿线显示——removeTierLabelArea 后修复（UC-02） | ADR-101, D-40 | `by-git` |
| T-03: Phoneme 缩放+刻度密与比例尺关联修复（UC-03） | ADR-102, D-24, D-26 | `by-git` |
| T-04: PhonemeEditor 工具栏异常按钮清理（UC-04） | ADR-103, D-43 | `by-git` |
| T-06: Chart widget 贯穿线拖动修复（UC-06） | ADR-105, D-41 | `by-git` |
| T-07: Slicer 文件列表异常按钮清理（UC-07） | ADR-106, D-38 | `by-git` |
| T-08: 导出页表格 DsTextDocument 回退读取（UC-08） | ADR-107 | `by-git` |
| T-09: GAME MIDI 音符长度修正——save/load 结束边界（UC-09） | ADR-108 | `by-git` ✅ |
| T-10: RMVPE 路径指向 ONNX 文件选择器 + Test 兼容（UC-10） | ADR-109 | `by-git` ✅ |
| T-11: Pitch 音频播放条 TotalDuration 回退（UC-11） | ADR-110 | `by-git` |
| T-12: Phoneme 缩放系统重新设计——Ctrl+滚轮全局可用（UC-12） | ADR-111, D-24, D-26 | `by-git` ✅ |
| T-13: Phoneme 标签区域数据隔离——仅显示 grapheme+phoneme（UC-13） | ADR-112 | `by-git` ✅ |
| T-14: Pitch 文件信息显示 + 音频时长三段回退（UC-14） | ADR-113 | `by-git` ✅ |
| T-15: Phoneme 贯穿线拖动修复——绘制+chart click 防平移（UC-15） | ADR-114, D-41 | `by-git` ✅ |
| T-P10-1~8: P-10 统一路径库（8 个子任务） | BUG-04/05/06/31, PATH-01~08 | `4c72815` |
| T-P11-1: `m_xxxRunning` 改为 `std::atomic<bool>` | P-11 | `9738f0f` |
| T-P11-2: 线程安全文档 + source() 保护 | P-11 | `9738f0f` + `4c72815` |
| T-P12-1: 批量处理模板提取到 EditorPageBase | P-12, TD-05 | `9738f0f` |
| T-P12-2: AudioChartWidget 共同基类提取 | P-12, TD-05 | `2e1756b` |

### 已废弃

| 任务 | 原因 |
|------|------|
| ~~T-05: MinLabel/Phoneme 复用 Slicer 文件列表（UC-05）~~ | 已撤回，导致文件列表共存 |
| ~~T-P12-3: DsSlicerPage 继承 SlicerPage~~ | 低优先级，暂缓 |