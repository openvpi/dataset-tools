# 魔法数字审计报告

> 本文档记录代码库中所有发现的可配置化魔法数字，按来源文件和分类整理。
>
> 最后更新：2025-05-21

---

## 1. 概览

对代码库进行了全面扫描，发现的魔法数字按领域分为以下几类：

| 类别 | 文件数 | 参数个数 | 配置优先级 |
|------|--------|---------|-----------|
| 频谱图渲染 | 1 | 5 | **高** |
| 功率图渲染 | 1 | 3 | **高** |
| 波形图渲染 | 1 | 3 | 中 |
| 钢琴卷帘编辑器 | 3 | 10 | 中 |
| 音高提取 | 3 | 5 | **高** |
| 通用常量 | 1 | 3 | 低（已有 Constants.h） |

---

## 2. 高频优先级 — 频谱图渲染参数

### 2.1 SpectrogramChartPanel

| 参数 | 当前硬编码值 | 位置 | 说明 | 建议配置名 |
|------|------------|------|------|-----------|
| `kStandardSampleRate` | `44100.0` | SpectrogramChartPanel.h:L35 | STFT 标准采样率（用于自适应 hop/window） | `spectrogram.sampleRate` |
| `kStandardHopSize` | `256` | SpectrogramChartPanel.h:L36 | STFT 标准 hop size（决定频谱时间分辨率） | `spectrogram.hopSize` |
| `kStandardWindowSize` | `2048` | SpectrogramChartPanel.h:L37 | STFT 标准窗口大小（决定频谱频率分辨率） | `spectrogram.windowSize` |
| `kMinIntensityDb` | `-120.0` | SpectrogramChartPanel.h:L38 | 频谱图最小强度阈值（dB），低于此值视为静默 | `spectrogram.minDb` |
| `kMaxIntensityDb` | `0.0` | SpectrogramChartPanel.h:L39 | 频谱图最大强度（dB），归一化上限 | `spectrogram.maxDb` |

**业务影响**：
- `windowSize` / `hopSize` 直接影响频谱图的**频率分辨率**和**时间分辨率**，是用户最可能需要调整的参数。
- `minDb` / `maxDb` 控制频谱图的**动态范围**显示效果。

---

## 3. 高频优先级 — 功率图渲染参数

### 3.1 PowerChartPanel

| 参数 | 当前硬编码值 | 位置 | 说明 | 建议配置名 |
|------|------------|------|------|-----------|
| `kWindowSize` | `2048` | PowerChartPanel.h:L28 | RMS 计算窗口大小（样本数） | `power.windowSize` |
| `kMinPower` | `-96.0f` | PowerChartPanel.h:L26 | 功率图最小 dB 值 | `power.minDb` |
| `kMaxPower` | `0.0f` | PowerChartPanel.h:L27 | 功率图最大 dB 值 | `power.maxDb` |

**业务影响**：
- `windowSize` 越大，RMS 平滑度越高，但瞬时变化越迟钝。

---

## 4. 中优先级 — 波形图渲染参数

### 4.1 WaveformChartPanel

| 参数 | 当前硬编码值 | 位置 | 说明 | 建议配置名 |
|------|------------|------|------|-----------|
| 响度归一化系数 | `0.5f` | WaveformChartPanel.cpp:L94 | RMS / 0.5 做响度 clamp（控制波形填充色彩映射） | `waveform.loudnessRef` |
| 振幅范围 | `[-1.0f, 1.0f]` | WaveformChartPanel.cpp:L177-178 | 归一化音频采样范围 | `waveform.ampRange` |
| 振幅缩放范围 | `[0.1, 20.0]` | ChartPanelBase.h:L92-93 | Shift+滚轮缩放振幅的 min/max | `waveform.ampZoomRange` |

---

## 5. 中优先级 — 钢琴卷帘编辑器参数

### 5.1 PianoRollView

| 参数 | 当前硬编码值 | 位置 | 说明 | 建议配置名 |
|------|------------|------|------|-----------|
| `boundaryHitRadius` | `5.0` | PianoRollView.cpp:L487 | 音符边界点击检测半径（像素） | `pianoroll.hitRadius` |
| 默认分辨率 | `40` | PianoRollView.cpp:L340 | resetZoom 时的默认分辨率 | `pianoroll.defaultResolution` |
| 垂直缩放 | `20.0` | PianoRollView.cpp:L350 | 钢琴卷帘垂直缩放因子 | `pianoroll.vScale` |

### 5.2 PianoRollRenderer / RenderState

| 参数 | 当前硬编码值 | 位置 | 说明 | 建议配置名 |
|------|------------|------|------|-----------|
| `PianoWidth` | `52` | PianoRollRenderer.h:L105 | 钢琴键盘宽度 | `pianoroll.pianoWidth` |
| `MinMidi` | `24` | PianoRollRenderer.h:L108 | 最低 MIDI 音符号 | `pianoroll.minMidi` |
| `MaxMidi` | `96` | PianoRollRenderer.h:L109 | 最高 MIDI 音符号 | `pianoroll.maxMidi` |
| `ScrollBarSize` | `14` | PianoRollRenderer.h:L107 | 滚动条宽度 | `pianoroll.scrollBarSize` |

### 5.3 PianoRollInputHandler

| 参数 | 当前硬编码值 | 位置 | 说明 | 建议配置名 |
|------|------------|------|------|-----------|
| `ModulationDragSensitivity` | `80.0` | PianoRollInputHandler.h:L155 | 调制拖动灵敏度 | `pianoroll.modSensitivity` |

---

## 6. 高频优先级 — 音高提取参数

### 6.1 CsvToDsConverter（已完成配置化 ✅）

| 参数 | 状态 | 说明 |
|------|------|------|
| `minF0 = 50.0f` | **已配置化** | F0 检测最小频率，存入 `Settings/pitchMinF0` |
| `maxF0 = 1100.0f` | **已配置化** | F0 检测最大频率，存入 `Settings/pitchMaxF0` |

### 6.2 PitchExtractionService

| 参数 | 当前硬编码值 | 位置 | 说明 | 建议配置名 |
|------|------------|------|------|-----------|
| 提取 timestep | `0.01f` | PitchExtractionService.cpp:L14 | RMVPE 音高提取时间步长（10ms） | `pitch.timestep` |
| hopSize（重采样后） | `512` | PitchExtractionService.cpp:L36 | F0 曲线重采样时的 hop size | `pitch.hopSize` |
| F0 缩放因子 | `1000.0f` | PitchExtractionService.cpp:L23 | RMVPE 原始 F0 值转换为 Hz 的倍率 | 不需要配置化 |

### 6.3 RMVPE 推理层

| 参数 | 当前值 | 来源 | 说明 |
|------|--------|------|------|
| RMVPE hop size | `160` | RMVPE 模型内部 | RMVPE 模型内部 hop size，来自模型设计，非本项目定义 |
| RMVPE window size | (模型相关) | RMVPE 模型内部 | 窗口大小由模型结构决定 |

---

## 7. 不需要配置化的值（分类依据）

以下值属于**数学常数、格式约定、UI 微调和渲染细节**，不建议配置化：

| 值 | 来源 | 分类 |
|----|------|------|
| `M_PI = 3.14159265358979323846` | SpectrogramChartPanel.cpp:L12 | 数学常数 |
| Blackman-Harris 窗口系数 | SpectrogramChartPanel.cpp:L97 | 数字信号处理标准系数 |
| `20.0 * log10(...)` | PowerChartPanel.cpp:L76 | dB 转换公式（20μPa 参考） |
| `10.0 * log10(...)` | SpectrogramChartPanel.cpp:L189 | 功率 dB 转换公式 |
| `1e-10`, `1e-20` | PowerPanel/SpectrogramPanel | 数值稳定性保护（非业务参数） |
| `0.35875, 0.48829, 0.14128, 0.01168` | SpectrogramChartPanel.cpp:L97 | 标准 Blackman-Harris 4 项窗口系数 |
| 波形画笔宽度 `0.5` | WaveformChartPanel.cpp:L204 | UI 渲染细节 |
| 功率画笔宽度 `1.5` | PowerChartPanel.cpp:L207 | UI 渲染细节 |
| 透明度 `40`, `60` | WaveformChartPanel.cpp:L175 | UI 渲染细节 |
| 默认视口分辨率 `40` | PianoRollView.cpp:L340 | 页面默认分辨率（已有 VIEW-16 机制） |

---

## 8. 汇总统计

| 图表类型 | 可配置参数数 | 已配置化 | 待配置化 |
|----------|------------|---------|---------|
| 频谱图 (Spectrogram) | 5 | 0 | 5 |
| 功率图 (Power) | 3 | 0 | 3 |
| 波形图 (Waveform) | 3 | 0 | 3 |
| 钢琴卷帘 (PianoRoll) | 10 | 0 | 10 |
| 音高提取 (Pitch) | 2 | 2 | 0 |
| **合计** | **23** | **2** | **21** |

---

## 9. 下一步：ChartConfig 体系设计

ChartConfig 系统将在 v5 阶段 2.1 实施，详见 zero-debt-v5-refactoring-plan.md 第6.1节。