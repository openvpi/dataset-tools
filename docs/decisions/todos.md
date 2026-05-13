# TODO 跟踪

## 待实施决策 (D-items)

| # | 任务 | 状态 | 备注 |
|---|---|---|---|
| D-42 | FA word.end边界实现 | ⚠ 部分完成 | buildFaLayers只输出word.start，word.end+phone[end].end绑定未实现 |
| D-43 | PitchLabeler工具栏+音频播放修复 | ❌ 待实施 | 创建独立QToolBar，修复onSliceSelectedImpl音频加载 |
| D-54 | 删除旧Widget死代码(WaveformWidget/SpectrogramWidget/PowerWidget) | ✅ 完成 | 文件已删除，注释引用已更新为 ChartPanel |
| — | 统一边界线渲染到BoundaryLineRenderer | ✅ 完成 | 创建 BoundaryLineRenderer 统一渲染器；BoundaryOverlayWidget、IntervalTierView 委托绘制；删除死代码 BoundaryLineLayer和BoundaryInteractionManager；移除ChartPanelBase::drawBoundaries()避免分隔线打断贯穿线 |

## 重构任务

### P0 -- 高优先级

| 任务 | 说明 | 工作量 | 状态 |
|---|---|---|---|
| EngineAliveToken统一归基类管理 | 各页面仍各自声明独立成员，未纳入EditorPageBase统一管理(map) | 0.3天 | ✅ |
| ensureEngine模板上提到EditorPageBase | Pitch已模板化，Phoneme+Min仍手动 | 1天 | ✅ |
| 清理旧WaveformWidget/SpectrogramWidget/PowerWidget死代码 | ~836行，见D-54 | 0.5天 | ✅ |

### P1 -- 中优先级

| 任务 | 说明 | 工作量 | 状态 |
|---|---|---|---|
| runAsyncTask异步框架提取 | 4个异步处理函数遵循same validate->flag->run->callback模式 | 2天 | ✅ |
| AudioEditorWidgetBase统一基类 | PhonemeEditor/PitchEditor~200-300行脚手架重复 | 3天 | ✅ |
| data-sources依赖倒置 | 直接链接5个infer lib编译时耦合(AD-01) | 2天 | ✅ |

### P2 -- 低优先级

| 任务 | 说明 | 工作量 | 状态 |
|---|---|---|---|
| FunASR适配器隔离 | 29文件~2400行，违反P-17 | 2周 | ❌ |
| ViewportManager/ViewportController重命名 | 同名近义易混淆 | 0.5天 | ✅ |
| QSettings键名统一(TD-20) | 部分硬编码->全部SettingsKey | 1天 | ✅ |
| SimpleDialogs清理 | 拆分RunProgressRow | 0.5天 | ✅ |

### P3 -- 长期

| 任务 | 说明 | 工作量 | 状态 |
|---|---|---|---|
| PitchLabelerPage拆分(1283行) | 最大源文件，违反P-01 | 2天 | ⚠ 部分完成 (提取了buildAlignInput、gameNoteMidiName、resolveAlignInputWithPhNum) |
| PhonemeLabelerPage精简(994行) | 提取applyFaResult | 1天 | ✅ |
| Spectrogram增量渲染(TD-03) | rebuildViewImage每次重建全图 | 1周 | ❌ |
| 未使用变量/编译警告清理 | halfWindow等 | 0.5天 | ✅ |
| QMetaMethod反射移除(TD-01) | 连接不可靠静默失败 | 0.5天 | ✅ |

## 实施顺序

### 迭代1 (补完快速修复 ~2天)

1. EngineAliveToken统一收归基类管理
2. ensureEngine模板上提EditorPageBase
3. 删除旧Widget死代码

### 迭代2 (核心重构 ~1周)

4. runAsyncTask框架提取
5. AudioEditorWidgetBase统一基类
6. data-sources依赖倒置

### 迭代3 (整合优化 ~1周)

7. ViewportManager/Controller重命名
8. QSettings键名统一
9. SimpleDialogs/PitchLabelerKeys清理

### 迭代4 (长期 ~2周)

10. FunASR适配器隔离
11. PitchLabelerPage拆分
12. QMetaMethod反射移除
13. Spectrogram增量渲染优化
