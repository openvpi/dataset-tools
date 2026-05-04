# Phoneme Editor UI 改进任务清单

> 更新时间：2026-05-03

---

## 已完成

### 任务 15: 默认比例 2:3:5

`PhonemeEditor::buildLayout()` 中 `m_rightSplitter` stretch factor 设为 waveform:2 / power:3 / spectrogram:5。

### 任务 16: TimeRuler 刻度密度降低

`TimeRulerWidget::paintEvent()` 中 `pixelsPerTick` 从 80 提升到 150，主刻度间距更大更易读。刻度通过共享 `ViewportController` 与波形图、频谱图完全绑定同步缩放。

### 任务 17: 波形图 Ctrl+滚轮振幅缩放 + dB Y 轴

- `WaveformWidget` 新增 `m_amplitudeScale`（范围 0.1–20.0）
- Ctrl+滚轮改为调整垂直振幅（原时间轴缩放由其他 widget 的 Ctrl+滚轮处理）
- `drawWaveform()` 中 sample 值乘以 `m_amplitudeScale` 并 clamp
- 新增 `drawDbAxis()` 在左侧绘制 0dB / -6dB / -12dB / -24dB 参考线

### 任务 18: 播放贯穿游标线

- `BoundaryOverlayWidget` 新增 `setPlayhead(double sec)` — 在 overlay 层绘制红色竖线贯穿所有图表
- `PhonemeEditor::connectSignals()` 中 `playheadChanged` 同时更新 WaveformWidget 和 BoundaryOverlay
- 播放停止后 200ms 无更新自动清除游标（QTimer 单次触发检测）
