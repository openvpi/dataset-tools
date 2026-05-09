# 重构方案

> 2026-05-09 — 新增 T-01~T-11 重构任务（UC-01~UC-11）
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
| 62 | 右键直接播放，不弹菜单 | 有效 |
| 65 | IBoundaryModel → AudioVisualizerContainer 扩展 | 有效 |
| 66 | LabelSuite 底层统一 dstext | 有效 |
| 69 | ISettingsBackend → 废弃 ProjectSettingsBackend | 有效 |
| 70 | FileDataSource 内部 dstext + FormatAdapter | 有效 |
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
| 100 | fa_grapheme 层代替直接替换 grapheme（UC-01） | **已完成** |
| 101 | 贯穿线显示修复——removeTierLabelArea 后 overlay 偏移（UC-02） | **已完成** |
| 102 | Phoneme 页面缩放与刻度同步修复（UC-03） | **已完成** |
| 103 | PhonemeEditor 工具栏异常按钮清理（UC-04） | **已完成** |
| 104 | MinLabel/Phoneme 复用 Slicer 文件列表功能（UC-05） | **已完成** |
| 105 | Chart widget 贯穿线拖动修复（UC-06） | **已完成** |
| 106 | Slicer 文件列表异常按钮清理（UC-07） | **已完成** |
| 107 | 导出页表格数据从 PipelineContext 读取正确数据（UC-08） | **已完成** |
| 108 | GAME MIDI 音符长度修正——save/load 结束边界（UC-09） | **已完成** ✅ |
| 109 | RMVPE 路径指向 ONNX 文件选择器 + Test 兼容（UC-10） | **已完成** ✅ |
| 110 | PitchEditor 音频播放条长度修复（UC-11） | **已完成** |

---

## 待办任务

### 已完成

| 任务 | 关联 | 提交 |
|------|------|------|
| FA grapheme 层替换（不再创建 fa_grapheme） | BUG-24, BUG-25, D-42 修订 | 2026-05-08 |
| PitchLabeler 音频播放修复 | BUG-26, D-43 修订 | 2026-05-08 |
| 移除 loadFromDsText 中 CJK 重命名逻辑 | BUG-32 | 2026-05-08 |
| PathUtils 新增 join/toUtf8/toWide 方法 | P-10, ADR-97 | `4c72815` |
| 废弃 dstools::toFsPath / DsDocument::toFsPath | P-10, ADR-98 | `4c72815` |
| JsonHelper 内部 pathToUtf8 改用 PathUtils::toUtf8 | P-10, ADR-99 | `4c72815` |
| T-P10-1~8: P-10 统一路径库（8 个子任务） | BUG-04/05/06/31, PATH-01~08, §4 | `4c72815` |
| T-P11-1: `m_xxxRunning` 改为 `std::atomic<bool>` | P-11 | `9738f0f` |
| T-P11-2: 线程安全文档 + source() 保护 | P-11 | `9738f0f` + `4c72815` |
| T-P12-1: 批量处理模板提取到 EditorPageBase | P-12, TD-05 | `9738f0f` |
| T-P12-2: AudioChartWidget 共同基类提取 | P-12, TD-05 | `2e1756b` |

| T-01: fa_grapheme 层代替直接替换 grapheme（UC-01） | ADR-100, D-42 回滚 | `by-git` |
| T-02: 贯穿线显示——removeTierLabelArea 后修复（UC-02） | ADR-101, D-40 | `by-git` |
| T-03: Phoneme 缩放+刻度密与比例尺关联修复（UC-03） | ADR-102, D-24, D-26 | `by-git` |
| T-04: PhonemeEditor 工具栏异常按钮清理（UC-04） | ADR-103, D-43 | `by-git` |
| T-05: ~~MinLabel/Phoneme 复用 Slicer 文件列表（UC-05）~~ **已撤回** | ADR-104, P-12, D-19 | 见下方 BUGFIX-05 |
| T-06: Chart widget 贯穿线拖动修复（UC-06） | ADR-105, D-41 | `by-git` |
| T-07: Slicer 文件列表异常按钮清理（UC-07） | ADR-106, D-38 | `by-git` |
| T-08: 导出页表格 DsTextDocument 回退读取（UC-08） | ADR-107 | `by-git` |
| T-09: GAME MIDI 音符长度修正——save/load 结束边界（UC-09） | ADR-108 | `by-git` ✅ |
| T-10: RMVPE 路径指向 ONNX 文件选择器 + Test 兼容（UC-10） | ADR-109 | `by-git` ✅ |
| T-11: Pitch 音频播放条 TotalDuration 回退（UC-11） | ADR-110 | `by-git` |

### 缺陷修复（2026-05-09 复核）

| 缺陷 | 描述 | 状态 |
|------|------|------|
| BUGFIX-01 | Phoneme 三层标签：`setTierReadOnly`/`autoDetectBindingGroups` 用 `doc.layers` 索引（3层）而非过滤后的 `layers`（2层），导致 setTierReadOnly(2) 越界展开 vector | ✅ 已修复 |
| BUGFIX-02 | 文件列表共存：T-05 加入 EditorPageBase 的 AudioFileListPanel 与各页面 SliceListPanel 并存两栏，已全部撤回 | ✅ 已修复 |
| BUGFIX-03 | MIDI 音符保存丢失结束边界：`saveCurrentSlice` 只写 start 边界而缺失 end 边界，导致重新加载时 note.duration 为相邻 note 间距 | ✅ 已修复 |
| BUGFIX-04 | RMVPE Test 按钮不兼容 .onnx 文件路径：`onTestModel` 将 modelPath 当目录（QDir），但 createOnnxModelConfigRow 选择的是文件 | ✅ 已修复 |

### 待执行

| 任务 | 关联 | 优先级 |
|------|------|--------|
| T-P12-3: DsSlicerPage 继承 SlicerPage | P-12, TD-11 | 低 |

### 已废弃

| 任务 | 关联 | 优先级 |
|------|------|--------|
| ~~LayerDependency 改用层名称引用~~ | ~~TD-21, ARCH-17~~ | ~~中~~ ✅ 已完成 |
| ~~DataSource 页面音频加载逻辑统一~~ | ~~TD-19/20, ARCH-16~~ | ~~中~~ ✅ 已完成 |
| ~~移除 loadFromDsText 中 CJK 重命名逻辑（修复三层问题）~~ | ~~BUG-32~~ | ~~高~~ ✅ 已完成 |
