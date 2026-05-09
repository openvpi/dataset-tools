# 重构方案

> 2026-05-09 — 新增 P-10/P-11/P-12 相关重构任务
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

---

## 待办任务

### 已完成

| 任务 | 关联 | 提交 |
|------|------|------|
| FA grapheme 层替换（不再创建 fa_grapheme） | BUG-24, BUG-25, D-42 修订 | 2026-05-08 |
| PitchLabeler 音频播放修复 | BUG-26, D-43 修订 | 2026-05-08 |
| 移除 loadFromDsText 中 CJK 重命名逻辑 | BUG-32 | 2026-05-08 |
| PathUtils 新增 join/toUtf8/toWide 方法 | P-10, ADR-97 | 2026-05-09 |
| 废弃 dstools::toFsPath / DsDocument::toFsPath | P-10, ADR-98 | 2026-05-09 |
| JsonHelper 内部 pathToUtf8 改用 PathUtils::toUtf8 | P-10, ADR-99 | 2026-05-09 |

### 待执行 — P-10 统一路径库

| 任务 | 关联 | 优先级 | 状态 |
|------|------|--------|------|
| T-P10-1: 替换推理层 `path.string()` 为 `PathUtils::toUtf8()` | BUG-04/05/06/31, PATH-01~08 | 高 | ✅ 已完成 |
| T-P10-2: 替换 `path.u8string()` 手动转换为 `dstools::pathToUtf8()` | P-10 | 高 | ✅ 已完成 |
| T-P10-3: 替换 PathResolvers.cpp 手动路径拼接为 `PathUtils::join()` | P-10 | 中 | ✅ 已完成 |
| T-P10-4: 替换 MinLabelService.cpp 手动路径拼接 | P-10 | 中 | ✅ 已完成 |
| T-P10-5: 替换 HubertAlignmentProcessor.cpp 中 `DsDocument::toFsPath` 调用 | P-10 | 高 | ✅ 已完成 |
| T-P10-6: 替换 DsDocument.cpp 内部 `toFsPath` 调用为 `PathUtils::toStdPath` | P-10 | 高 | ✅ 已完成 |
| T-P10-7: 替换 `qWarning()`/`qDebug()`/`std::cerr` 为 `DSFW_LOG_*`（推理层） | P-10, §4 | 中 | ✅ 已完成 |
| T-P10-8: 替换 `qWarning()`/`qDebug()`/`std::cerr` 为 `DSFW_LOG_*`（框架层） | P-10, §4 | 中 | ✅ 已完成 |

### 待执行 — P-11 多线程安全

| 任务 | 关联 | 优先级 | 状态 |
|------|------|--------|------|
| T-P11-1: MinLabelPage/PitchLabelerPage/PhonemeLabelerPage 的 `m_xxxRunning` 改为 `std::atomic<bool>` | P-11 | 中 | ✅ 已完成 |
| T-P11-2: 后台线程访问 `source()` 方法时添加线程安全保护 | P-11 | 中 | ✅ 已完成 |

### 待执行 — P-12 相似模块统一

| 任务 | 关联 | 优先级 | 状态 |
|------|------|--------|------|
| T-P12-1: 提取批量处理模板到 EditorPageBase | P-12, TD-05 | 中 | ✅ 已完成（模板已实现，子类迁移待后续） |
| T-P12-2: 提取 chart widget 鼠标交互逻辑到共同基类 | P-12, TD-05 | 中 | ✅ 已完成（AudioChartWidget 基类已创建，WaveformWidget 已迁移，SpectrogramWidget/PowerWidget 待后续迁移） |
| T-P12-3: DsSlicerPage 继承 SlicerPage | P-12, TD-11 | 低 | 待执行 |

### 已废弃

| 任务 | 关联 | 优先级 |
|------|------|--------|
| ~~LayerDependency 改用层名称引用~~ | ~~TD-21, ARCH-17~~ | ~~中~~ ✅ 已完成 |
| ~~DataSource 页面音频加载逻辑统一~~ | ~~TD-19/20, ARCH-16~~ | ~~中~~ ✅ 已完成 |
| ~~移除 loadFromDsText 中 CJK 重命名逻辑（修复三层问题）~~ | ~~BUG-32~~ | ~~高~~ ✅ 已完成 |
