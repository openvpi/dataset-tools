# 重构方案

> 2026-05-08 — 合并自 `refactoring-plan-v3.md` + `refactoring-roadmap-v2.md`
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

---

## 待办任务

### 已完成

| 任务 | 关联 | 提交 |
|------|------|------|
| FA grapheme 层替换（不再创建 fa_grapheme） | BUG-24, BUG-25, D-42 修订 | 2026-05-08 |
| PitchLabeler 音频播放修复 | BUG-26, D-43 修订 | 2026-05-08 |

### 待执行

| 任务 | 关联 | 优先级 |
|------|------|--------|
| ~~LayerDependency 改用层名称引用~~ | ~~TD-21, ARCH-17~~ | ~~中~~ ✅ 已完成 |
| ~~DataSource 页面音频加载逻辑统一~~ | ~~TD-19/20, ARCH-16~~ | ~~中~~ ✅ 已完成 |

---
