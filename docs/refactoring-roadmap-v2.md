# 重构路线图与任务清单

> 2026-05-04
>
> 依赖关系决定执行顺序。同一 Phase 内的任务可并行。
> 前置文档：[human-decisions.md](human-decisions.md)、[refactoring-v2.md](refactoring-v2.md)

---

## Phase Q — 紧急修复（P0）

无依赖，立即可做。

### Q.1 Release 构建 DLL 缺失修复

- [x] 为 DsLabeler/LabelSuite 的 exe targets 添加 POST_BUILD 命令：使用 `$<TARGET_RUNTIME_DLLS:tgt>` 拷贝所有运行时 DLL 到 `CMAKE_RUNTIME_OUTPUT_DIRECTORY`
- [x] 确认 vcpkg SHARED 依赖（cpp-pinyin、fftw3、sndfile、soxr、SDL2、mpg123）的 DLL 被拷贝
- [x] 修复 DsLabeler CMakeLists 的 `WIN32_EXECETABLE` 拼写错误 → `WIN32_EXECUTABLE`
- [x] 验证 `deploy.cmake` 的 install 逻辑：glob `CMAKE_RUNTIME_OUTPUT_DIRECTORY/*.dll` 能覆盖所有运行时 DLL（CI 使用 install target 部署）
- [x] 减少不必要的中间文件：确保 STATIC lib 的 .lib/.a 不被 install，仅安装 exe + shared DLL + 资源
- [x] 验证：本地从 `cmake-build-release/bin/` 直接运行不闪退
- [ ] 验证：CI `cmake --build build --target install` 后 deploy/ 目录完整可用

### Q.2 Slicer 音频列表和切点持久化

- [x] `.dsproj` 格式扩展：`slicer.audioFiles` + `slicer.slicePoints` + `slicer.params`
- [x] `DsProject` 增加 slicer 数据的序列化/反序列化
- [x] `DsSlicerPage` 在切点变更时自动保存到工程
- [x] `DsSlicerPage::onActivated()` 从工程加载音频列表和切点
- [ ] 验证：关闭重开工程后 slicer 状态完整恢复

### Q.3 CMake 架构整理

#### Q.3.1 IDE FOLDER 分组修复

- [x] 审查 `src/CMakeLists.txt` 和各子目录的 `CMAKE_FOLDER` 设置
- [x] 确保 framework 下所有 targets（dsfw-base、dsfw-core、dsfw-ui-core、dstools-audio、dsfw-widgets、dstools-infer-common）在 `Libraries/Framework`
- [x] 确保 domain（dstools-domain）在 `Libraries`
- [x] 确保 ui-core（dstools-ui-core）在 `Libraries`
- [x] 确保 widgets（dstools-widgets）在 `Libraries`
- [x] 确保 infer 下所有 targets（audio-util、game-infer、rmvpe-infer、hubert-infer、FunAsr）在 `Libraries/Infer`
- [x] 确保 libs 下所有 targets 在 `Libraries/Libs`
- [x] 确保 apps 下的 exe targets 在根目录或 `Applications`
- [x] 确保 tests 在 `Tests`
- [ ] CLion 重新加载 CMake 验证 target 树结构

#### Q.3.2 Install 精简（减少不必要的中间文件）

- [x] 审查 `dstools_add_library()` 中 EXPORT_SET 带来的 `install(TARGETS ...)` 规则——STATIC 库不需要 install ARCHIVE 到 deploy 目录
- [x] `deploy.cmake` 仅拷贝 exe + SHARED DLL + 资源（dict/、translations/、plugins/）到 install prefix
- [x] 去掉对 .lib/.a 静态库文件的 install（它们只在 `find_package(dsfw)` 场景下有用，但 CI artifact 不需要）
- [x] 分离 `install` 为两种模式：
  - **DEPLOY**（CI/用户打包用）：仅 exe + DLL + 资源 → 最小体积
  - **SDK**（开发者用，`find_package(dsfw)`）：额外包含 headers + .lib + cmake config
- [ ] 可选：在 `dstools_add_executable()` 的 DEPLOY 模式中使用 CMake 3.21+ 的 `install(RUNTIME_DEPENDENCY_SET)` 自动收集运行时依赖，替代手动 glob

#### Q.3.3 CI workflow 完善

- [x] CI workflow 中 `cmake --install` 步骤加上 `-DDSTOOLS_DEPLOY_MODE=ON`，确保 artifact 不包含 .lib/.a
- [x] 修复 release.yml 中 vcpkg 缺少自定义 triplets
- [x] 添加 deploy artifact 验证步骤（检查无 .lib/.a 文件）
- [ ] 可选：添加 `find_package(dsfw)` 验证步骤（单独 job，不使用 DEPLOY_MODE）

---

## Phase R — 配置架构重设计

依赖：Q.2（.dsproj 格式变更需要协调）

### R.1 配置迁移到用户目录

- [x] 废弃 `ProjectSettingsBackend`
- [x] `ISettingsBackend` 仅保留 `AppSettingsBackend`
- [x] DsLabeler Settings 页切换到 `AppSettingsBackend`
- [x] `.dsproj` `defaults` 字段废弃（读取时迁移，不再写入）
- [x] `SlicerConfig` 从 `defaults.slicer` 迁移到 `slicer.params`
- [x] `ExportConfig` 从 `defaults.export` 迁移到顶层 `export`
- [x] 版本号升级到 3.1.0
- [x] `ds-format.md` 更新

### R.2 模型懒加载

- [x] 实现 `LazyModelManager`（首次使用时加载，配置变更时 invalidate）
- [x] 替换所有直接加载模型的代码为 `LazyModelManager::getOrLoad()`
- [x] Settings 页模型路径变更 → `LazyModelManager::invalidate()`
- [ ] 验证：启动时不加载任何模型，首次 FA/F0/MIDI 时自动加载

### R.3 图表比例 + 子图顺序持久化

- [x] `AppSettings` 增加各页面 splitter ratio 和 chartOrder 的存取
- [x] splitter 拖动后自动保存比例
- [x] 启动时恢复上次保存的比例和子图顺序
- [ ] 验证：跨工程保持比例和排列

---

## Phase S — AudioVisualizerContainer 可视化架构重设计

依赖：R.3（比例/顺序持久化）

### S.1 AudioVisualizerContainer 基类

- [x] 新建 `AudioVisualizerContainer` 基类（`src/apps/shared/audio-visualizer/`）
- [x] 垂直布局管理：MiniMapScrollBar → TimeRuler → TierLabelArea → QSplitter(子图)
- [x] 子图注册 API：`addChart(id, widget, defaultOrder)` / `removeChart(id)`
- [x] 子图顺序可配置（从 AppSettings 读取 chartOrder）
- [x] 共享 ViewportController + IBoundaryModel
- [x] BoundaryOverlayWidget 覆盖 TierLabelArea + QSplitter 区域
- [x] 验证：可动态添加/移除子图，顺序可调整

### S.2 TierLabelArea 标签区域

- [x] `TierLabelArea` 基类：`activeTierIndex()` / `activeBoundaries()` / `activeTierChanged` 信号
- [x] `SliceTierLabel`（Slicer 用）：单层，自动编号 "1", "2", "3"...
- [x] `PhonemeTextGridTierLabel`（PhonemeLabeler 用）：多层 TextGrid 编辑
  - [x] 最左侧竖排 radio button 组（每层一个）
  - [x] 选中层的边界线向下贯穿所有子图
  - [x] 非选中层的边界线仅在标签区域内，从该层向下延伸到最低层截止
- [x] 验证：radio button 切换时边界线贯穿范围正确变化

### S.3 MiniMapScrollBar 组件

- [ ] 新建 `MiniMapScrollBar` widget（迷你波形缩略图 + 滑窗）
- [ ] 绘制完整音频 peak envelope
- [ ] 滑窗拖动 → 更新 ViewportController offset
- [ ] 滑窗边缘拖动 → 更新 ViewportController zoom
- [ ] 移除底部 QScrollBar
- [ ] 移除顶部 PlayWidget 工具栏（PlayWidget 保留为内部播放后端）

### S.4 刻度线重设计

- [ ] 参考 ds-editor-lite `ITimelinePainter` 实现基于时间的刻度算法
- [ ] 层级：1h / 10min / 1min / 10s / 1s / 100ms / 10ms
- [ ] `minimumSpacing = 24px`，渐显渐隐（`spacingVisibility` + `smoothStep`）
- [ ] 替换现有 `TimeRulerWidget::paintEvent()` 实现
- [ ] 确保所有子图共享 ViewportController 不允许独立横向缩放

### S.5 边界线贯穿规则实现

- [ ] `BoundaryOverlayWidget` 覆盖 TierLabelArea + QSplitter 整个区域
- [ ] 选中层边界线：从标签区域该层顶部 → 最底部子图底部
- [ ] 非选中层边界线：从该层顶部 → 标签区域最低层底部
- [ ] 红色播放游标：始终贯穿标签区域 + 所有子图
- [ ] 播放结束 200ms 后游标消失
- [ ] 验证：Slicer 单层全贯穿；PhonemeLabeler radio 切换时贯穿范围正确

### S.6 层级包含规则与拖动约束

- [ ] 高层级边界强制对齐低层级边界（words 的边界必须与某个 phones 边界重合）
- [ ] 拖动约束：边界线不允许超越同层级内相邻的边界线（clamp 到 (L, R) 范围）
- [ ] Slicer 同理：切割线拖动不可越过相邻切割线
- [ ] 已有 `IntervalTierView::updateDrag()` 和 `WaveformWidget::updateBoundaryDrag()` 需要增加 clamp 逻辑
- [ ] 验证：拖动到极限位置时被正确约束

### S.7 键鼠交互统一

- [ ] Ctrl+滚轮 → 横向缩放（所有子图同步）
- [ ] Shift+滚轮 → 波形图垂直振幅（仅 WaveformWidget）
- [ ] 滚轮 → 横向滚动
- [ ] 右键单击任意子图 → 播放当前分割区域（基于选中层的边界）
- [ ] 确保频谱图/功率图不能独立缩放时间轴

### S.8 y 轴刻度清理

- [ ] 波形图：保留左侧 dB 参考刻度（0/-6/-12/-24dB），去掉横向虚线
- [ ] 频谱图/功率图：移除 y 轴刻度和虚线

### S.9 迁移现有页面到 AudioVisualizerContainer

- [ ] DsSlicerPage 改用 AudioVisualizerContainer + SliceTierLabel
- [ ] PhonemeLabelerPage 改用 AudioVisualizerContainer + PhonemeTextGridTierLabel
- [ ] 删除旧的独立布局代码
- [ ] 验证：两个页面功能无回归

---

## Phase T — 功能增强

依赖：S（可视化架构完成后）

### T.1 DsLabeler CSV 预览页

- [ ] ExportPage 内部增加 TabWidget："导出设置" | "预览数据"
- [ ] "预览数据" tab：调用 CsvAdapter 生成内存 CSV
- [ ] QTableView + QStandardItemModel 只读展示
- [ ] 缺少前置数据的行标红

### T.2 快捷键系统

- [ ] 定义各页面快捷键映射表（Slicer: S=自动切片, E=导出; PhonemeLabeler: F=FA; PitchLabeler: F=音高, M=MIDI; MinLabel: R=ASR）
- [ ] 各页面在 `createMenuBar()` 中为 QAction 设置快捷键
- [ ] 工具按钮 tooltip 标注快捷键
- [ ] AppShell 页面切换时激活/停用对应快捷键组
- [ ] 验证：菜单中显示快捷键，快捷键功能正确

### T.3 Settings 页子图顺序配置 UI

- [ ] Settings 页增加"图表顺序"配置项
- [ ] 可拖拽排序的列表（waveform / power / spectrogram / viseme 等）
- [ ] 变更后实时更新 AudioVisualizerContainer

---

## Phase U — 文档清理

可与任何 Phase 并行。

### U.1 已完成的删除

- [x] 删除 `phase-l-implementation.md`
- [x] 删除 `phoneme-editor-improvements.md`
- [x] 删除 `refactoring-roadmap.md`（有价值的 ADR 表和 Phase P 完成记录已迁移到 refactoring-v2.md §6-7）

### U.2 待完成的合并/更新

- [ ] `build.md` + `local-build.md` → 合并为单个 `build.md`
- [ ] `unified-app-design.md` §9 更新配置持久化（引用 D-01）
- [ ] `unified-app-design.md` §11 更新可视化架构（引用 refactoring-v2.md §2）
- [x] `ds-format.md` §2 去掉 defaults 字段

---

## 执行顺序总结

```
Q.1 ──────┐
Q.2 ──────┤─→ R.1 → R.2 ─┐
Q.3.1 ────┤                │
Q.3.2 ────┤    R.3 ───────┤─→ S.1 → S.2 → S.3 → S.4 → S.5 → S.6 → S.7 → S.8 → S.9 ─→ T.1 → T.2 → T.3
Q.3.3 ────┘                │
                           │
U.1/U.2 ──────────────────┘ (并行)
```

**预估工作量**：

| Phase | 复杂度 | 预估天数 |
|-------|--------|---------|
| Q (紧急修复) | 低-中 | 1-2 天 |
| R (配置架构) | 中 | 2-3 天 |
| S (可视化架构，9 个子任务) | 高 | 8-12 天 |
| T (功能增强) | 中 | 3-4 天 |
| U (文档清理) | 低 | 0.5 天 |
| **合计** | | **14-21 天** |
