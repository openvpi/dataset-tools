# 魔法数字审计报告

> 本文档记录代码库中所有发现的可配置化魔法数字，按来源文件和分类整理。
>
> 最后更新：2026-06-04

---

## 1. 概览

对代码库进行了全面扫描，发现的魔法数字按领域分为以下几类：

| 类别 | 文件数 | 参数个数 | 配置状态 |
|------|--------|---------|---------|
| 频谱图渲染 | 1 | 5 | **已完成** ✅ (ChartConfigRegistry) |
| 功率图渲染 | 1 | 3 | **已完成** ✅ (ChartConfigRegistry) |
| 波形图渲染 | 1 | 3 | **已完成** ✅ (ChartConfigRegistry) |
| 钢琴卷帘编辑器 | 3 | 10 | **已完成** ✅ (C-01: ChartConfigRegistry) |
| 音高提取 | 3 | 5 | minF0/maxF0 已配置 ✅ |
| 通用常量 | 1 | 3 | 低（Constants.h 已有） |

---

## 2. 高频优先级 — 频谱图渲染参数 ✅ 已完成

### 2.1 SpectrogramChartPanel

> **状态**：已配置化。参数通过 `ChartConfigRegistry` 管理，在 `SpectrogramChartPanel::registerChartConfig()` 中注册，
> 在 `loadConfigParams()` 中加载到 `m_configHopSize`/`m_configWindowSize`/`m_configMinDb`/`m_configMaxDb`。

| 参数 | 默认值 | 配置键 | 说明 |
|------|--------|--------|------|
| hopSize | `256` | `spectrogram.hopSize` | STFT hop size（决定频谱时间分辨率） |
| windowSize | `2048` | `spectrogram.windowSize` | STFT 窗口大小（决定频谱频率分辨率） |
| minDb | `-80.0` | `spectrogram.minDb` | 频谱图动态范围下限 |
| maxDb | `0.0` | `spectrogram.maxDb` | 频谱图动态范围上限 |

`kStandardSampleRate = 44100.0` 为数学标准常数，不需要配置化。

---

## 3. 高频优先级 — 功率图渲染参数 ✅ 已完成

### 3.1 PowerChartPanel

> **状态**：已配置化。参数通过 `ChartConfigRegistry` 管理，在 `PowerChartPanel::registerChartConfig()` 中注册，
> 在 `loadConfigParams()` 中加载到 `m_configWindowSize`/`m_configMinDb`/`m_configMaxDb`。

| 参数 | 默认值 | 配置键 | 说明 |
|------|--------|--------|------|
| windowSize | `2048` | `power.windowSize` | RMS 计算窗口大小（样本数） |
| minDb | `-96.0` | `power.minDb` | 功率图最小 dB 值 |
| maxDb | `0.0` | `power.maxDb` | 功率图最大 dB 值 |

---

## 4. 中优先级 — 波形图渲染参数 ✅ 已完成

### 4.1 WaveformChartPanel

> **状态**：已配置化。参数通过 `ChartConfigRegistry` 管理，在 `WaveformChartPanel::registerChartConfig()` 中注册，
> 在 `loadConfigParams()` 中加载到 `m_loudnessRef`/`m_opacity`。

| 参数 | 默认值 | 配置键 | 说明 |
|------|--------|--------|------|
| 响度归一化系数 | `0.5` | `waveform.loudnessRef` | RMS / 此值做响度 clamp |
| 填充透明度 | `0.16` | `waveform.opacity` | 波形填充区域透明度 |

振幅范围 `[-1.0, 1.0]` 为归一化音频标准范围，不需要配置化。
振幅缩放范围 `[0.1, 20.0]` 在 `ChartPanelBase` 中作为 UI 交互参数，不需要配置化。

---

## 5. 中优先级 — 钢琴卷帘编辑器参数 ✅ 已完成 (C-01)

> **状态**：已配置化（commit `26169c73`）。所有 10 个参数通过 `ChartConfigRegistry` 管理，
> 在 `PianoRollChartPanel::registerChartConfig()` 中注册。

### 5.1 PianoRollView

| 参数 | 默认值 | 配置键 | 说明 |
|------|--------|--------|------|
| boundaryHitRadius | `5.0` | `pianoroll.hitRadius` | 音符边界点击检测半径（像素） |
| 默认分辨率 | `40` | `pianoroll.defaultResolution` | resetZoom 时的默认分辨率 |
| 垂直缩放 | `20.0` | `pianoroll.vScale` | 钢琴卷帘垂直缩放因子 |

### 5.2 PianoRollRenderer / RenderState

| 参数 | 默认值 | 配置键 | 说明 |
|------|--------|--------|------|
| PianoWidth | `52` | `pianoroll.pianoWidth` | 钢琴键盘宽度 |
| MinMidi | `24` | `pianoroll.minMidi` | 最低 MIDI 音符号 |
| MaxMidi | `96` | `pianoroll.maxMidi` | 最高 MIDI 音符号 |
| ScrollBarSize | `14` | `pianoroll.scrollBarSize` | 滚动条宽度 |

### 5.3 PianoRollInputHandler

| 参数 | 默认值 | 配置键 | 说明 |
|------|--------|--------|------|
| ModulationDragSensitivity | `80.0` | `pianoroll.modSensitivity` | 调制拖动灵敏度 |

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

| 图表类型 | 可配置参数数 | 已配置化 | 配置方式 |
|----------|------------|---------|---------|
| 频谱图 (Spectrogram) | 5 | 5 | ChartConfigRegistry |
| 功率图 (Power) | 3 | 3 | ChartConfigRegistry |
| 波形图 (Waveform) | 3 | 3 | ChartConfigRegistry |
| 钢琴卷帘 (PianoRoll) | 10 | 10 | ChartConfigRegistry (C-01) |
| 音高提取 (Pitch) | 2 | 2 | AppSettings / SettingsKey |
| **合计** | **23** | **23** | **全部完成** |

---

## 9. 实施状态

ChartConfig 体系已全面实施。所有图表参数通过 `ChartConfigRegistry` 管理，支持运行时修改和持久化存储。
各图表的 `registerChartConfig()` 在应用启动时调用，`loadConfigParams()` 在构造函数中加载配置值。