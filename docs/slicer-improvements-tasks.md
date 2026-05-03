# Slicer 改进任务清单

基于用户需求整理的 DsSlicerPage / DroppableFileListPanel / WaveformPanel 改进任务。

---

## 任务 1: AudioFileListPanel 顶部增加操作按钮

**现状**: `DroppableFileListPanel` 的按钮（+、📁、−）位于列表**下方** (`DroppableFileListPanel.cpp:32-48`)。

**需求**: 文件列表**上端**增加以下按钮：
- 📁 打开文件夹
- `+` 添加文件
- `−` 移除选中
- `Discard` 丢弃当前文件（标记跳过，不从列表移除）
- `Clear` 清空列表

**实现方案**:
- 修改 `DroppableFileListPanel.cpp` 将 `btnLayout` 从 `layout->addLayout(btnLayout)` 移到列表上方
- 新增 `m_btnDiscard` 和 `m_btnClear` 按钮
- `Discard` 设置当前文件为灰色/删除线样式，记录到 discarded 集合
- `Clear` 调用现有 `clear()` 方法

**涉及文件**:
- `src/framework/widgets/src/DroppableFileListPanel.cpp`
- `src/framework/widgets/include/dsfw/widgets/DroppableFileListPanel.h`

---

## 任务 2: AudioFileListPanel 下端增加按个数显示的进度条

**现状**: `FileProgressTracker` 已存在，格式为 `"%1 / %2 已标注 (%3%)"` (SliceListPanel) 或 `"%1 / %2 已标注"` (DsSlicerPage)。目前 DsSlicerPage 中已设置 `ProgressBarStyle`。

**需求**: 进度条按**个数**显示（如 "12 / 50"），不要百分比。

**实现方案**:
- 确认 `DsSlicerPage::buildLayout()` 中的 `setFormat` 使用不含 `%3%` 的格式
- 已有 `setFormat(QStringLiteral("%1 / %2 已标注"))` — 确认 ProgressBarStyle 模式下数字正确显示
- 如果 `FileProgressTracker::ProgressBarStyle` 仍显示百分比，需修改其 `paintEvent` 改为仅显示数值

**涉及文件**:
- `src/framework/widgets/include/dsfw/widgets/FileProgressTracker.h`
- `src/framework/widgets/src/FileProgressTracker.cpp`（如需修改渲染逻辑）
- `src/apps/ds-labeler/DsSlicerPage.cpp`（确认格式字符串）

---

## 任务 3: Slicer 切片后固定为 dsitem 走后续流程

**现状**: `DsSlicerPage::onExportAudio()` 导出后创建 `PipelineContext` 并通过 `project->setItems()` 注册切片为项目条目。但导出使用的命名是 `prefix_001.wav` 格式（prefix 默认为 "slice"）。

**需求**: 切片后的片段应固定为 `.dsitem` 格式，走后续标注/处理流程。

**实现方案**:
- 在 `onExportAudio()` 中，导出 WAV 后同时为每个切片生成 `.dsitem` 文件
- 使用 `DsItemManager::save()` 或直接写 PipelineContext JSON
- 确保 `ProjectDataSource` 能基于这些 dsitem 文件正确加载切片列表
- 切片 ID 和文件名保持一致

**涉及文件**:
- `src/apps/ds-labeler/DsSlicerPage.cpp` (`onExportAudio`)
- `src/domain/include/dstools/DsItemRecord.h`
- `src/domain/src/DsItemManager.cpp`

---

## 任务 4: TimeRuler 刻度跟随缩放自动调整（刻度太密）

**现状**: `WaveformPanel::TimeRuler::paintEvent()` 已经有自适应逻辑——使用 `80.0 / m_pixelsPerSecond` 计算 `secPerTick`，然后从 `niceSteps[]` 中选择合适的间隔。

**需求**: 当前刻度太密，应该跟随缩放比例自动调整间距。

**实现方案**:
- 增大目标刻度间距，从 `80.0` 改为更大的值（如 `120.0` 或 `150.0`），使刻度不那么密
- 或者根据当前 widget 宽度动态计算合适的目标像素间距
- 考虑在 `niceSteps` 数组中增加更粗粒度的选项

**涉及文件**:
- `src/apps/shared/waveform-panel/WaveformPanel.cpp` (TimeRuler::paintEvent, 第 48 行附近)

---

## 任务 5: Slicer 右击播放当前切片（而非从头播放）

**现状**: `WaveformPanel::connectSignals()` 中 `rightClickPlay` 信号的处理逻辑 (第 586-602 行) 已经实现了"找到右击位置所在的 segment 并播放"的逻辑：
```cpp
connect(m_waveformDisplay, &WaveformDisplay::rightClickPlay, this,
    [this](double clickTime) {
        double segStart = 0.0;
        double segEnd = totalDuration();
        for (const auto &b : m_boundaries) {
            if (b.timeSec <= clickTime)
                segStart = b.timeSec;
            if (b.timeSec > clickTime) {
                segEnd = b.timeSec;
                break;
            }
        }
        m_playback->playSegment(segStart, segEnd);
    });
```

**问题**: 实际行为是从头播放。可能原因：
1. `rightClickPlay` 信号未正确触发（被 `boundaryRightClicked` 拦截了）
2. `PlaybackController::playSegment()` 实现有 bug
3. 在 Knife 模式或 Pointer 模式下右击行为不同

**实现方案**:
- 检查 `WaveformDisplay` 的鼠标事件处理，确认右击时是否正确区分"点击 boundary"和"点击空白区域"
- 检查 `PlaybackController::playSegment()` 是否正确 seek 到 startSec
- 确保右击空白区域时播放当前所在切片段

**涉及文件**:
- `src/apps/shared/waveform-panel/WaveformPanel.cpp` (WaveformDisplay 鼠标事件)
- `src/apps/shared/waveform-panel/PlaybackController.cpp`

---

## 任务 6: Slicer 导出音频命名为 `原文件名_xxx.扩展名`

**现状**: `DsSlicerPage::onExportAudio()` 第 451 行：
```cpp
QString filename = QStringLiteral("%1_%2.wav").arg(prefix).arg(i + 1, digits, 10, QChar('0'));
```
其中 `prefix` 来自 `SliceExportDialog::prefix()`，默认为 "slice"。

**需求**: 输出文件名应为 `原音频文件的fullname_xxx.扩展名`：
- `fullname` = 原音频文件的完整文件名（不含扩展名）
- `xxx` = 序号，位数(fill)可调
- 扩展名 = 原音频文件的扩展名（或导出格式的扩展名）

**实现方案**:
- 在 `onExportAudio()` 中获取当前音频文件路径：`m_audioFileList->currentFilePath()`
- 使用 `QFileInfo(audioPath).completeBaseName()` 作为 prefix
- 使用原文件扩展名或根据导出格式决定扩展名
- `SliceExportDialog` 中的 prefix 字段默认填充为原文件名，digits 字段已可调

**涉及文件**:
- `src/apps/ds-labeler/DsSlicerPage.cpp` (`onExportAudio`)
- `src/apps/ds-labeler/SliceExportDialog.cpp` (默认值设置)

---

## 任务 7: 切出的 dsitem 在切换其他工具页面时可用

**现状**: `DsMinLabelPage` 等页面通过 `SliceListPanel` + `ProjectDataSource` 获取切片列表。`SliceListPanel::refresh()` 从 `m_source->sliceIds()` 读取。`ProjectDataSource::sliceIds()` 从 `DsProject::items()` 中提取。

**需求**:
- 切换到其他页面（MinLabel、PhonemeLabeler 等）时，Slicer 产生的切片应自动可用
- 默认打开**第一个**条目或**正在标注的那个**

**当前问题**: 切片在其他页面切换条目时不能正常使用。

**可能原因**:
1. `onExportAudio()` 中 `project->setItems()` 后，其他页面的 `SliceListPanel` 没有收到 `sliceListChanged` 信号（页面未 activate 时未连接）
2. `ProjectDataSource::audioPath()` 返回的路径不正确（相对路径解析问题）
3. 其他页面的 `onActivated()` 没有正确刷新 SliceListPanel
4. `onSliceSelected()` 中加载音频文件失败（路径问题）

**实现方案**:
- 确保页面 `onActivated()` 时调用 `m_sliceList->refresh()` 
- 确保 `ProjectDataSource::audioPath()` 返回的路径是绝对路径或可正确解析
- 默认选中逻辑：如果有正在标注的 slice（有 dirty/incomplete 状态），选中它；否则选中第一个
- 检查 `DsMinLabelPage::onActivated()` 是否调用了 refresh + 选中首条

**涉及文件**:
- `src/apps/ds-labeler/DsMinLabelPage.cpp` (`onActivated`)
- `src/apps/ds-labeler/DsPhonemeLabelerPage.cpp` (`onActivated`)
- `src/apps/ds-labeler/DsPitchLabelerPage.cpp` (`onActivated`)
- `src/apps/ds-labeler/SliceListPanel.cpp` (`refresh`)
- `src/apps/ds-labeler/ProjectDataSource.cpp` (`audioPath`, `sliceIds`)

---

## 实施优先级

| 优先级 | 任务 | 复杂度 | 状态 |
|--------|------|--------|------|
| P0 | 任务 7: 切片在其他页面可用（核心功能 bug） | 中 | ✅ 完成 |
| P0 | 任务 5: 右击播放当前切片 | 低-中 | ✅ 完成 |
| P1 | 任务 6: 导出命名为原文件名_xxx | 低 | ✅ 完成 |
| P1 | 任务 3: 切片后固定为 dsitem | 中 | ✅ 完成 |
| P1 | 任务 4: TimeRuler 刻度密度调整 | 低 | ✅ 完成 |
| P2 | 任务 1: 文件列表上端操作按钮 | 低 | ✅ 完成 |
| P2 | 任务 2: 进度条按个数显示 | 低 | ✅ 完成 |
