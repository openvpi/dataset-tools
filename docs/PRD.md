# DiffSinger Dataset Tools - Product Requirements Document (PRD)

**Version**: 2.0  
**Date**: 2026-04-26  
**Project**: dataset-tools  
**Organization**: Team OpenVPI  
**License**: Apache 2.0  

---

## 1. 产品概述

### 1.1 产品定位
DiffSinger Dataset Tools 是一套面向歌声合成（Singing Voice Synthesis, SVS）数据集制作的桌面工具集，为 DiffSinger 训练数据的标注、对齐、切片和转录提供端到端的 GUI 工作流。

### 1.2 目标用户
- 歌声合成研究人员与开发者
- 语音数据集标注人员
- 音频工程师和声乐爱好者
- DiffSinger / OpenVPI 生态用户

### 1.3 技术栈
| 层级 | 技术 |
|---|---|
| 语言 | C++17 |
| UI 框架 | Qt 6.8+ (Widgets), CI 使用 Qt 6.9.3 |
| 构建系统 | CMake 3.17+, vcpkg |
| AI 推理 | ONNX Runtime (CPU / CUDA / DirectML) |
| 音频处理 | libsndfile, soxr, FFmpeg, SDL2, mpg123, FLAC |
| 平台 | Windows (主要), macOS, Linux |

---

## 2. 产品架构

### 2.1 系统架构图

```
┌──────────────────────────────────────────────────────────────┐
│                      GUI Applications                        │
├──────────┬───────────┬────────────┬─────────┬───────────────┤
│ MinLabel │SlurCutter │AudioSlicer │ LyricFA │ HubertFA      │
│ (标注)   │ (MIDI编辑)│ (音频切片) │(歌词对齐)│(音素对齐)     │
├──────────┴───────────┴────────────┼─────────┴───────────────┤
│         GameInfer (音频转MIDI)     │                         │
├───────────────────────────────────┴─────────────────────────┤
│                    Shared Libraries                          │
├──────────┬───────────┬────────────┬─────────┬───────────────┤
│audio-util│game-infer │some-infer  │rmvpe-   │ FunAsr        │
│          │           │            │infer    │               │
├──────────┼───────────┴────────────┴─────────┤               │
│ qsmedia  │  AsyncTaskWindow                  │               │
├──────────┼──────────────────────────────────┤               │
│ffmpeg-   │  sdlplayback                     │               │
│decoder   │                                  │               │
├──────────┴──────────────────────────────────┴───────────────┤
│              External: ONNX Runtime, Qt6, SDL2, FFmpeg       │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 详细依赖关系图

```
                         ┌──────────┐
                         │ Qt6 Core │
                         │ Widgets  │
                         │   Svg    │
                         └────┬─────┘
                              │
              ┌───────────────┼───────────────────────────────┐
              │               │                               │
        ┌─────┴─────┐  ┌─────┴──────┐                 ┌──────┴──────┐
        │  QsMedia   │  │ audio-util │                 │AsyncTaskWin │
        │  (static)  │  │  (shared)  │                 │  (shared)   │
        │ Qt::Core   │  │ SndFile    │                 │ Qt::Core    │
        └──┬────┬────┘  │ soxr,mpg123│                 │ Qt::Widgets │
           │    │       │ FLAC,xsimd │                 └──────┬──────┘
     ┌─────┴┐  ┌┴──────┐└──┬───┬────┘                        │
     │FFmpeg│  │ SDL    │   │   │                              │
     │Decode│  │Playback│   │   │                              │
     └──┬───┘  └──┬────┘   │   │                              │
        │         │        │   │                              │
  ┌─────┴─────────┴──┐    │   │    ┌──────────┐    ┌────────┴──────┐
  │    MinLabel      │    │   │    │  FunAsr   │    │   LyricFA     │
  │ +cpp-pinyin      │    │   │    │+onnxruntime│   │+AsyncTaskWin  │
  │ +cpp-kana        │    │   │    │+fftw3     │    │+audio-util    │
  │ +qBreakpad       │    │   │    └─────┬─────┘    │+FunAsr        │
  └──────────────────┘    │   │          │          │+cpp-pinyin    │
                          │   │          │          └───────────────┘
  ┌───────────────────┐   │   │    ┌─────┴──────────────────────────┐
  │   SlurCutter      │   │   │    │       HubertFA                 │
  │ +SDLPlayback      │   │   │    │+AsyncTaskWin +audio-util       │
  │ +FFmpegDecoder    │   │   │    │+onnxruntime +nlohmann_json     │
  └───────────────────┘   │   │    └────────────────────────────────┘
                          │   │
  ┌───────────────────┐   │   │    ┌────────────────────────────────┐
  │  AudioSlicer      │   │   ├───>│     game-infer (shared)        │
  │ +SndFile only     │   │   │    │+audio-util +wolf-midi          │
  └───────────────────┘   │   │    │+nlohmann_json +onnxruntime     │
                          │   │    └───────────┬────────────────────┘
                          │   │                │
                          │   │          ┌─────┴────────┐
                          │   │          │  GameInfer    │
                          │   │          │+game-infer    │
                          │   │          │+dxgi (Win)    │
                          │   │          └──────────────┘
                          │   │
                          │   ├───>some-infer (+audio-util +wolf-midi +onnxruntime)
                          │   └───>rmvpe-infer (+audio-util +onnxruntime)
                          │
                     (onnxruntime: pre-built at src/libs/onnxruntime/)
```

### 2.3 目录结构
```
dataset-tools/
├── CMakeLists.txt              # 根构建文件 (C++17, 输出路径配置)
├── README.md                   # 项目文档
├── LICENSE                     # Apache 2.0
├── .clang-format               # 代码格式化规范
├── setup-vcpkg-temp.bat        # vcpkg 引导脚本
├── .github/workflows/
│   └── windows-rel-build.yml   # CI: 标签触发 Windows Release 构建
├── scripts/
│   ├── cmake/
│   │   ├── utils.cmake         # CMake 工具宏
│   │   └── winrc.cmake         # Windows RC 资源生成
│   ├── setup-onnxruntime.cmake # ONNX Runtime 下载脚本
│   └── vcpkg-manifest/
│       └── vcpkg.json          # vcpkg 依赖清单
├── src/
│   ├── CMakeLists.txt          # 查找 Qt6, 添加 apps/libs 子目录
│   ├── apps/                   # 6 个 GUI 应用
│   │   ├── MinLabel/           # 音频标注工具 (v0.0.1.8)
│   │   ├── SlurCutter/         # DiffSinger 句子/MIDI 编辑器
│   │   ├── AudioSlicer/        # 自动音频切片
│   │   ├── LyricFA/            # 歌词强制对齐 (v0.0.0.2)
│   │   ├── HubertFA/           # Hubert 音素强制对齐 (v0.0.0.1)
│   │   └── GameInfer/          # GAME 音频转 MIDI (v0.0.0.1)
│   └── libs/                   # 8+ 共享库
│       ├── audio-util/         # 音频工具 (v1.0.0.0)
│       ├── game-infer/         # GAME 推理引擎 (v1.0.0.0)
│       ├── some-infer/         # SOME MIDI 估计推理 (v1.0.0.0)
│       ├── rmvpe-infer/        # RMVPE 基频估计推理 (v1.0.0.0)
│       ├── FunAsr/             # FunASR 语音识别
│       ├── AsyncTaskWindow/    # 批处理任务窗口基类
│       ├── qsmedia/            # 音频媒体抽象层
│       ├── ffmpegdecoder/      # FFmpeg 解码插件
│       ├── sdlplayback/        # SDL 播放插件
│       └── onnxruntime/        # 预编译 ONNX Runtime
└── vcpkg/                      # vcpkg 安装 (gitignored)
```

---

## 3. 功能模块详细说明

### 3.1 MinLabel — 音频标注工具

**版本**: 0.0.1.8  
**入口**: `src/apps/MinLabel/main.cpp:22-65`

#### 3.1.1 功能需求

| ID | 功能 | 描述 | 优先级 |
|---|---|---|---|
| ML-001 | 目录浏览 | 打开目录，按文件树展示 wav/mp3/m4a/flac 音频文件 | P0 |
| ML-002 | 音频播放 | 支持播放/暂停/停止/进度拖拽，可选音频设备 | P0 |
| ML-003 | JSON 标注 | 编辑关联 JSON 文件中的 lab (音素) 和 raw_text (原文) 字段 | P0 |
| ML-004 | G2P 转换 | 支持普通话拼音、粤语粤拼、日语罗马字的字素到音素转换 | P0 |
| ML-005 | Lab 转 JSON | 批量将 .lab 文件转换为项目 JSON 文件 | P1 |
| ML-006 | 数据导出 | 导出音频 + lab 文件 + raw_text 到指定目录，支持去除声调 | P1 |
| ML-007 | 标注进度 | 自动统计已标注文件数/总文件数的进度百分比 | P2 |
| ML-008 | 快捷键配置 | 通过 INI 配置文件自定义快捷键 | P2 |

#### 3.1.2 数据模型
```json
{
  "lab": "ni3 hao3",
  "raw_text": "你好",
  "lab_without_tone": "ni hao",
  "isCheck": true
}
```

#### 3.1.3 数据流

```
用户打开目录
  → openDirectory() [MainWindow.cpp:191]
  → QFileSystemModel 过滤 *.wav/*.mp3/*.m4a/*.flac

选择文件
  → _q_treeCurrentChanged() [MainWindow.cpp:427]
  → 自动保存上一文件 → openFile() [MainWindow.cpp:196]
  → readJsonFile() [Common.cpp:223] 读取 <basename>.json
  → 显示 lab/raw_text 到 TextWidget

G2P 转换
  → _q_textToPronunciation() [TextWidget.cpp:104]
  → 普通话: g2p_man->hanziToPinyin() [TextWidget.cpp:109]
  → 日语: Kana::kanaToRomaji() [TextWidget.cpp:118]
  → 粤语: g2p_canton->hanziToPinyin() [TextWidget.cpp:125]

保存文件
  → saveFile() [MainWindow.cpp:210]
  → 写入 JSON (lab + raw_text + lab_without_tone + isCheck)
  → 写入 .lab 文件 (纯音素文本)
  → 声调去除: static QRegularExpression("[^a-z]")

导出
  → ExportDialog → exportAudio() [MainWindow.cpp:439]
  → mkCopylist() [Common.cpp] 生成复制清单
  → 复制 wav/lab/raw_text/lab_without_tone 到输出目录
```

#### 3.1.4 关键类

| 类名 | 文件 | 职责 |
|---|---|---|
| `Minlabel::MainWindow` | `gui/MainWindow.h:13` | 主窗口：文件浏览树、菜单栏、标注进度 |
| `Minlabel::TextWidget` | `gui/TextWidget.h` | G2P 转换编辑器 (pinyin/romaji/jyutping) |
| `Minlabel::PlayWidget` | `gui/PlayWidget.h` | 音频播放控件 (FFmpeg解码 + SDL播放) |
| `Minlabel::ExportDialog` | `gui/ExportDialog.h` | 导出配置对话框 |
| `Minlabel::Common` | `gui/Common.h` | 工具函数 (JSON读写/文件复制/路径转换) |

#### 3.1.5 配置项

**文件**: `<app_dir>/config/MinLabel.ini`

| 键 | 默认值 | 说明 |
|---|---|---|
| `Shortcuts/Open` | `Ctrl+O` | 打开目录快捷键 |
| `Shortcuts/Export` | `Ctrl+E` | 导出快捷键 |
| `Navigation/Prev` | `PgUp` | 上一文件 |
| `Navigation/Next` | `PgDown` | 下一文件 |
| `Playback/Play` | `F5` | 播放/暂停 |
| `General/LastDir` | (空) | 上次打开的目录 |

---

### 3.2 SlurCutter — DiffSinger 句子编辑器

**入口**: `src/apps/SlurCutter/main.cpp:18-47`

#### 3.2.1 功能需求

| ID | 功能 | 描述 | 优先级 |
|---|---|---|---|
| SC-001 | DS 文件加载 | 加载 DiffSinger .ds JSON 文件，解析句子数组 | P0 |
| SC-002 | F0 可视化 | 在钢琴卷帘上绘制 F0 曲线（红色）和 MIDI 音符 | P0 |
| SC-003 | 音符编辑 | 拖拽调整音符音高，支持半音/音分级精度 | P0 |
| SC-004 | 连音 (Slur) 标记 | 分割音符创建连音，合并连音到前一音符 | P0 |
| SC-005 | 滑音 (Glide) 标记 | 设置音符的上滑/下滑装饰风格 | P1 |
| SC-006 | 句子导航 | 在文件内/跨文件切换句子，自动保存 | P0 |
| SC-007 | 音频播放 | 按句子范围播放音频，平滑播放头 | P1 |
| SC-008 | 缩放/滚动 | Ctrl+滚轮垂直缩放，Ctrl+Shift 水平缩放，拖拽滚动 | P1 |
| SC-009 | 音素显示 | 在音符下方显示音素文本和时间 | P2 |
| SC-010 | 编辑追踪 | 记录已编辑文件列表到 SlurEditedFiles.txt | P2 |

#### 3.2.2 数据模型 (DiffSinger Sentence)

```json
{
  "text": "你 好",
  "ph_seq": "n i h ao",
  "ph_dur": "0.1 0.2 0.1 0.3",
  "ph_num": "2 2",
  "note_seq": "C4 D4",
  "note_dur": "0.3 0.4",
  "note_slur": "0 0",
  "note_glide": "none none",
  "f0_seq": "261.6 293.7 ...",
  "f0_timestep": 0.005
}
```

#### 3.2.3 数据流

```
打开文件
  → openFile() [MainWindow.cpp:225]
  → audioFileToDsFile() [MainWindow.cpp:43] 获取对应 .ds 文件
  → loadDsContent() [MainWindow.cpp:319] 解析 JSON 数组
  → 填充 sentenceWidget 和 dsContent (QVector<QJsonObject>)

选择句子
  → _q_sentenceChanged() [MainWindow.cpp:513]
  → f0Widget->setDsSentenceContent() [F0Widget.cpp:140]
  → 解析 ph_seq/ph_dur/note_seq/note_dur/note_slur/f0_seq/note_glide
  → 构建 IntervalTree<MiniNote> 用于空间查询

用户编辑音符
  → mousePressEvent/mouseMoveEvent/mouseReleaseEvent
  → splitNoteUnderMouse() [F0Widget.cpp:449] — 分割音符
  → shiftDraggedNoteByPitch() [F0Widget.cpp:478] — 调整音高
  → setDraggedNoteGlide() [F0Widget.cpp:510] — 设置滑音
  → mergeCurrentSlurToLeftNode() [F0Widget.cpp:555] — 合并连音

保存
  → pullEditedMidi() [MainWindow.cpp:278] — 从 F0Widget 提取修改
  → saveFile() [MainWindow.cpp:245] — 序列化为缩进 JSON 写入 .ds
  → 追加文件名到 SlurEditedFiles.txt
```

#### 3.2.4 F0Widget 内部结构 (1097 行)

| 功能分类 | 方法 | 行数 | 占比 |
|---|---|---|---|
| **渲染** | `paintEvent()` | 602-921 (~320行) | 29% |
| **输入处理** | `wheelEvent`, `mouseMoveEvent`, `mousePressEvent`, `mouseReleaseEvent`, `mouseDoubleClickEvent`, `contextMenuEvent` | ~110行 | 10% |
| **数据管理** | `setDsSentenceContent`, `clear`, `getSavedDsStrings`, `splitNoteUnderMouse`, `shiftDraggedNoteByPitch`, `setDraggedNotePitch`, `setDraggedNoteGlide`, `convertAllRestsToNormal`, `mergeCurrentSlurToLeftNode` | ~250行 | 23% |
| **菜单/UI** | 构造函数(菜单), `setMenuFromCurrentNote`, `toggleCurrentNoteRest`, `setCurrentNoteGlideType` | ~50行 | 5% |
| **工具函数** | `NoteNameToMidiNote`, `MidiNoteToNoteName`, `FrequencyToMidiNote`, `PitchToNotePlusCentsString`, `mouseOnNote` | ~60行 | 5% |
| **配置/状态** | `loadConfig`, `pullConfig`, `setPlayHeadPos`, `setErrorStatusText` | ~30行 | 3% |

#### 3.2.5 关键类

| 类名 | 文件 | 职责 |
|---|---|---|
| `SlurCutter::F0Widget` | `gui/F0Widget.h:14` | 核心钢琴卷帘编辑器 |
| `MiniNote` | `gui/F0Widget.h:100-113` | 音符: duration, pitch, cents, text, phones, isSlur, isRest, GlideStyle |
| `MiniPhone` | `gui/F0Widget.h:96-99` | 音素: ph, begin, duration |
| `GlideStyle` | `gui/F0Widget.h:51-55` | 滑音枚举: None / Up / Down |
| `ReturnedDsString` | `gui/F0Widget.h:27-32` | 序列化输出: note_seq, note_dur, note_slur, note_glide |
| `SlurCutter::MainWindow` | `gui/MainWindow.h:15` | 主窗口 |
| `SlurCutter::DsSentence` | `gui/DsSentence.h:10` | DS 句子结构 |

#### 3.2.6 配置项

**文件**: `<app_dir>/config/SlurCutter.ini`

| 键 | 默认值 | 说明 |
|---|---|---|
| `Shortcuts/Open` | `Ctrl+O` | 打开目录 |
| `Navigation/Prev` | `PgUp` | 上一句/文件 |
| `Navigation/Next` | `PgDown` | 下一句/文件 |
| `Playback/Play` | `F5` | 播放/暂停 |
| `General/LastDir` | (空) | 上次目录 |
| `F0Widget/snapToKey` | — | 吸附到半音 |
| `F0Widget/showPitchTextOverlay` | — | 显示音高文本 |
| `F0Widget/showPhonemeTexts` | — | 显示音素文本 |
| `F0Widget/showCrosshairAndPitch` | — | 显示十字线和音高 |

---

### 3.3 AudioSlicer — 音频切片工具

**入口**: `src/apps/AudioSlicer/main.cpp:11-34`

#### 3.3.1 功能需求

| ID | 功能 | 描述 | 优先级 |
|---|---|---|---|
| AS-001 | 静音检测 | 基于 RMS 能量的流式静音检测，可配置阈值 (-40dB) | P0 |
| AS-002 | 自动切片 | 在静音点分割音频文件，输出多个片段 | P0 |
| AS-003 | 参数配置 | 阈值、最小长度、最小间隔、跳跃大小、最大静音保留 | P0 |
| AS-004 | 批量处理 | 支持拖放添加多个 WAV 文件，批量切片 | P1 |
| AS-005 | 标记导出 | 保存/加载 Audacity CSV 格式标记文件 | P1 |
| AS-006 | 输出格式 | 支持 16/24/32-bit PCM 和 32-bit float WAV 输出 | P2 |
| AS-007 | 进度显示 | Windows 任务栏进度条集成 (ITaskbarList3) | P2 |

#### 3.3.2 核心算法

**RMS 静音检测 (`slicer/slicer.cpp:108`)**:

1. 参数转换: 时间参数 (ms) → 采样数 (`divIntRound()`)
2. 流式 RMS: `MovingRMS` 固定窗口滑动队列 (`slicer.cpp:22-34`)
3. 静音区域扫描: RMS 低于阈值的连续帧区间
4. 分割点选择: `argmin_range_view()` 找到最安静帧
5. 三种策略:
   - 静音 ≤ maxSilKept → 在中点分割
   - 静音 ≤ 2×maxSilKept → 在中间区域最安静处分割
   - 静音 > 2×maxSilKept → 两端各保留 maxSilKept

#### 3.3.3 数据流

```
用户添加 WAV 文件 (拖放/浏览)
  → 存储为 QListWidgetItem (UserRole+1 = 文件路径)

配置参数 → slot_start() [mainwindow.cpp:204]
  → 为每个文件创建 WorkThread (QRunnable)
  → QThreadPool (maxThreadCount=1) 串行执行

WorkThread::run()
  → SndfileHandle 读取音频
  → Slicer(threshold, minLength, minInterval, hopSize, maxSilKept)
  → slicer.slice() → MarkerList [(start, end), ...]
  → 写入切片 WAV 文件 / 保存 CSV 标记

信号回调
  → oneFinished(filename) → slot_oneFinished()
  → oneFailed(filename, msg) → slot_oneFailed()
```

---

### 3.4 LyricFA — 歌词强制对齐工具

**版本**: 0.0.0.2  
**入口**: `src/apps/LyricFA/main.cpp:11-55`

#### 3.4.1 功能需求

| ID | 功能 | 描述 | 优先级 |
|---|---|---|---|
| LF-001 | ASR 语音识别 | 使用 FunASR Paraformer 模型识别中文歌词 | P0 |
| LF-002 | 歌词匹配 | 基于 Needleman-Wunsch 算法对齐 ASR 结果与参考歌词 | P0 |
| LF-003 | 批量处理 | 支持批量处理音频文件夹 | P0 |
| LF-004 | 差异高亮 | 可视化显示 ASR 与参考歌词的差异 | P1 |
| LF-005 | 滑动窗口匹配 | LCS + 编辑距离的多策略最佳匹配搜索 | P1 |
| LF-006 | 中文处理 | 中文字符清洗、拼音转换 | P0 |

#### 3.4.2 双阶段数据流

**阶段一: ASR 语音识别**
```
slot_loadModel() [LyricFAWindow.cpp:246]
  → 加载 model/ParaformerAsrModel/ (model.onnx + vocab.txt)
  → 创建 Asr 对象

runTask() [LyricFAWindow.cpp:99]
  → 为每个 WAV 创建 AsrThread (QRunnable)
  → QThreadPool (maxThreadCount=1)

AsrThread::run() → Asr::forward()
  → audio-util: resample to 16kHz mono
  → AudioUtil::Slicer 切片 (拒绝 >60s 片段)
  → FunAsr::ModelImp::forward() [paraformer_onnx.cpp:107]:
    1. FeatureExtract: FFT(FFTW3) → 80-dim mel spectrogram
    2. apply_lfr(): 6x 下采样 + 7帧上下文 → 560-dim
    3. apply_cmvn(): 全局均值方差归一化 (hardcoded coefficients)
    4. ONNX 推理: [1,T,560] + [1] → logits[1,T',8404]
    5. greedy_search(): argmax over 8404 vocab → 文本

输出: .lab 文件 (识别文本)
```

**阶段二: 歌词匹配**
```
slot_matchLyric() [LyricFAWindow.cpp:170]
  → 为每个 .lab 创建 FaTread (QRunnable)

FaTread::run() → MatchLyric::match()
  → 文件名前缀匹配歌词行
  → ChineseProcessor::clean_text() 清洗
  → SequenceAligner::compute_alignment() — Needleman-Wunsch DP
  → SequenceAligner::find_best_match() — 滑动窗口搜索:
    1. 精确匹配
    2. scan_windows(): LCS 候选筛选 (OVERLAP_THRESHOLD=0.3)
    3. 编辑距离精炼

输出: .json (raw_text + lab)
```

#### 3.4.3 核心算法

| 算法 | 位置 | 复杂度 | 说明 |
|---|---|---|---|
| Needleman-Wunsch | `SequenceAligner.cpp: compute_alignment()` | O(n×m) 空间 | 全局对齐 + 回溯 |
| 编辑距离 | `SequenceAligner.cpp: compute_edit_distance()` | O(min(n,m)) 空间 | 空间优化版 |
| LCS | `SequenceAligner.cpp: compute_lcs_length()` | O(n×m) | 近似窗口评分 |
| 最佳匹配搜索 | `SequenceAligner.cpp: find_best_match()` | — | 精确 → LCS → 编辑距离 三级策略 |
| 频率重叠过滤 | `SequenceAligner.cpp: scan_windows()` | — | OVERLAP_THRESHOLD = 0.3 |

---

### 3.5 HubertFA — Hubert 音素强制对齐

**版本**: 0.0.0.1  
**入口**: `src/apps/HubertFA/main.cpp:9-46`

#### 3.5.1 功能需求

| ID | 功能 | 描述 | 优先级 |
|---|---|---|---|
| HF-001 | ONNX 推理 | 加载 HuBERT 模型，输出音素帧概率和边界概率 | P0 |
| HF-002 | Viterbi 对齐 | 基于动态规划的帧级音素对齐解码 | P0 |
| HF-003 | G2P 字典 | 基于字典的多语言字素到音素转换 | P0 |
| HF-004 | TextGrid 输出 | 生成 Praat TextGrid 格式的对齐结果 | P0 |
| HF-005 | 非词汇音素 | 检测呼吸音 (AP)、静音 (SP) 等非词汇音素 | P1 |
| HF-006 | 多语言支持 | 支持按语言选择 G2P 字典 | P1 |
| HF-007 | GPU 加速 | 支持 DirectML (Windows) 和 CUDA 执行提供程序 | P1 |

#### 3.5.2 数据流

```
slot_loadModel() [HubertFAWindow.cpp:197]
  → check_configs(): 验证 model.onnx/config.json/vocab.json
  → 创建 HFA 对象:
    → 加载 config.json (mel_spec_config)
    → 加载 vocab.json (dictionaries + non_lexical_phonemes)
    → 创建 DictionaryG2P (按语言)
    → 创建 AlignmentDecoder
    → 创建 NonLexicalDecoder

runTask() → HfaThread::run()
  → HFA::recognize(wavPath, language, non_speech_ph):
    1. 音频重采样 → HfaModel ONNX 推理
       输出: ph_frame_logits[3D], ph_edge_logits[2D], cvnt_logits[3D]
    2. DictionaryG2P: 文本 → ph_seq + word_seq + ph_idx_to_word_idx
    3. AlignmentDecoder: Viterbi DP (sigmoid边界 + softmax帧) → 帧级对齐
    4. NonLexicalDecoder: 检测非词汇音素 (AP/SP)
    5. WordList 构建:
       → add_AP() 添加呼吸音
       → add_SP() 添加静音
       → fill_small_gaps() 填充间隙
       → clear_language_prefix() 清理语言前缀

输出: TextGrid (phones tier + words tier)
```

#### 3.5.3 数据模型

```
WordList → Word[] → Phone[]
  Word: { start, end, text, phones[], log[] }
  Phone: { start, end, text }

方法:
  - add_AP(): 在词间插入呼吸音
  - add_SP(): 在词间插入静音段
  - fill_small_gaps(): 填充小间隙 (<0.1s)
  - check(): 验证时间连续性和排序
```

---

### 3.6 GameInfer — GAME 音频转 MIDI

**版本**: 0.0.0.1  
**入口**: `src/apps/GameInfer/main.cpp:5-17`

#### 3.6.1 功能需求

| ID | 功能 | 描述 | 优先级 |
|---|---|---|---|
| GI-001 | 模型加载 | 加载 GAME 4模型 ONNX pipeline | P0 |
| GI-002 | 音频推理 | WAV/FLAC/MP3 → MIDI 音符序列 | P0 |
| GI-003 | MIDI 输出 | 输出标准 MIDI 文件 (Format 1, PPQ 480) | P0 |
| GI-004 | 执行提供程序 | CPU / CUDA / DirectML 推理加速 | P1 |
| GI-005 | 参数配置 | 分割阈值、估计阈值、D3PM 步数、语言、速度 | P1 |
| GI-006 | GPU 选择 | DirectML 模式下 DXGI GPU 枚举 | P2 |

#### 3.6.2 GAME 推理管线

```
onExportMidiTask() [MainWidget.cpp:553]
  → QtConcurrent::run (后台线程)

Game::get_midi() [Game.cpp:94]:
  1. resample_to_vio() → 重采样到模型采样率
  2. AudioUtil::Slicer 切片 (threshold=0.02)
  3. Per chunk → GameModel::forward() [GameModel.cpp:234]:
     a. T = ceil(duration / timestep)
     b. runEncoder(): waveform[1,N] + duration[1] + language[1]
        → x_seg[1,T,D], x_est[1,T,D], maskT[1,T]
     c. runSegmenter(): 迭代 D3PM 去噪 (d3pm_ts 步)
        → boundaries[1,T]
     d. runBd2dur(): boundaries → durations[1,N], maskN[1,N]
     e. runEstimator(): x_est + boundaries + masks
        → presence[1,N] (bool), scores[1,N] (MIDI pitch float)
  4. build_midi_note() [Game.cpp:59]:
     presence > 0.5 → emit note, pitch = round(scores[i])
  5. makeMidiFile() [Game.cpp:27]: wolf-midi 写入 .mid

输出: MIDI 文件 (Format 1, PPQ 480, 2 tracks)
```

#### 3.6.3 配置项

**文件**: `<app_dir>/config/GameInfer.ini`

| 键 | 默认值 | 说明 |
|---|---|---|
| `MainWidget/modelPath` | `<app_dir>/model/GAME-1.0.3-small-onnx` | 模型目录 |
| `MainWidget/wavPath` | (空) | 输入音频路径 |
| `MainWidget/outMidiPath` | (空) | 输出 MIDI 路径 |
| `MainWidget/segThreshold` | `0.2` | 分割阈值 |
| `MainWidget/segRadiusFrame` | `2` | 分割半径 (帧) |
| `MainWidget/estThreshold` | `0.2` | 估计阈值 |
| `MainWidget/segD3PMNSteps` | `3` (index) | D3PM 步数下拉索引 |
| `MainWidget/max_audio_seg_length` | `60` | 最大音频段长度 (s) |

**模型配置** `config.json`:

| 键 | 默认值 | 说明 |
|---|---|---|
| `timestep` | `0.01` | 每帧秒数 |
| `samplerate` | `44100` | 目标采样率 |
| `embedding_dim` | `256` | 嵌入维度 |
| `languages` | `{name: id}` | 语言名→ID映射 |

---

## 4. ONNX 推理管线汇总

### 4.1 模型参数对照

| 模型 | 应用 | 输入张量 | 输出张量 | 采样率 |
|---|---|---|---|---|
| GAME Encoder | GameInfer | waveform[1,N], duration[1], language[1] | x_seg[1,T,D], x_est[1,T,D], maskT[1,T] | 44100 |
| GAME Segmenter | GameInfer | x_seg, known_boundaries, prev_boundaries, language, maskT, threshold, radius, t | boundaries[1,T] | — |
| GAME Bd2dur | GameInfer | boundaries[1,T], maskT[1,T] | durations[1,N], maskN[1,N] | — |
| GAME Estimator | GameInfer | x_est, boundaries, maskT, maskN, threshold | presence[1,N], scores[1,N] | — |
| SOME | (库) | waveform[1,N] | note_midi[1,K], note_rest[1,K], note_dur[1,K] | 44100 |
| RMVPE | (库) | waveform[1,N], threshold[1] | f0[1,K], uv[1,K] | 16000 |
| Paraformer | LyricFA | speech[1,T,560], speech_lengths[1] | logits[1,T',8404], encoder_out_lens[1] | 16000 |
| HuBERT | HubertFA | (from config) | ph_frame_logits[3D], ph_edge_logits[2D], cvnt_logits[3D] | (config) |

### 4.2 ONNX Session 公共配置

```cpp
// GameModel.cpp:170-172, SomeModel.cpp:20, 等
session_options.SetInterOpNumThreads(4);
session_options.SetGraphOptimizationLevel(ORT_ENABLE_EXTENDED);

// DirectML (Windows):
session_options.DisableMemPattern();
session_options.SetExecutionMode(ORT_SEQUENTIAL);
OrtSessionOptionsAppendExecutionProvider_DML(session_options, device_id);

// CUDA:
OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, &cuda_options);
```

---

## 5. 音频插件架构 (QsMedia)

### 5.1 接口层

| 接口 | 文件 | 关键虚方法 |
|---|---|---|
| `IAudioDecoder` | `qsmedia/Api/IAudioDecoder.h:12` | `open()`, `close()`, `Read()`, `SetPosition()` (继承 `WaveStream`) |
| `IAudioPlayback` | `qsmedia/Api/IAudioPlayback.h:13` | `setup()`, `dispose()`, `play()`, `stop()`, `setDecoder()`, `drivers()`, `devices()` |
| `IAudioEncoder` | `qsmedia/Api/IAudioEncoder.h:10` | (Stub — 无方法) |

### 5.2 工厂/插件加载

- `AudioDecoderFactory`: 扫描 `libraryPaths()/audiodecoders/` 加载插件 (环境变量: `QSAPI_AUDIO_DECODER`, 默认 `"FFmpegDecoder"`)
- `AudioPlaybackFactory`: 扫描 `libraryPaths()/audioplaybacks/` 加载插件 (环境变量: `QSAPI_AUDIO_PLAYBACK`, 默认 `"SDLPlayback"`)

### 5.3 实际使用方式

MinLabel 和 SlurCutter **直接实例化**而非通过工厂:
```cpp
// PlayWidget.cpp:204-205
decoder = new FFmpegDecoder();
playback = new SDLPlayback();
```

### 5.4 SDL 播放线程模型

```
主线程 (Qt UI)
  ├── SDL 轮询线程 (std::thread) — SDLPlaybackPrivate::poll()
  │   └── 无限循环处理 SDL 事件 (自定义事件类型)
  └── SDL 音频回调线程 — workCallback()
      └── 从 PCM 缓冲区读取数据 (mutex 保护)
```

---

## 6. 线程模型汇总

| 应用 | UI 线程 | 工作线程 | 通信方式 |
|---|---|---|---|
| **MinLabel** | 所有逻辑 | SDL poll + SDL audio callback | — |
| **SlurCutter** | 所有逻辑 | SDL poll + SDL audio callback | `playheadChanged` signal |
| **AudioSlicer** | UI + 结果处理 | `QThreadPool` (max 1) → `WorkThread` | `oneFinished`/`oneFailed` signals |
| **LyricFA** | UI + 结果处理 | `QThreadPool` (max 1) → `AsrThread`/`FaTread` | `oneFinished`/`oneFailed` signals |
| **HubertFA** | UI + 结果处理 | `QThreadPool` (max 1) → `HfaThread` | `oneFinished`/`oneFailed` signals |
| **GameInfer** | Qt Widgets | `QtConcurrent::run` (单后台线程) | `QMetaObject::invokeMethod(Qt::QueuedConnection)` |

---

## 7. 构建与部署

### 7.1 构建要求
- CMake >= 3.17 (推荐 3.20+)
- MSVC 2022 (Windows) / GCC / Clang
- Qt 6.8+
- vcpkg 包管理器

### 7.2 CMake 构建选项

| 选项 | 默认值 | 说明 |
|---|---|---|
| `BUILD_TESTS` | `ON` | 编译测试可执行文件 |
| `ONNXRUNTIME_ENABLE_DML` | `ON` (Windows) | 启用 DirectML GPU 加速 |
| `ONNXRUNTIME_ENABLE_CUDA` | `OFF` | 启用 CUDA GPU 加速 |
| `GAME_INFER_BUILD_STATIC` | `OFF` | 静态库构建 |
| `SOME_INFER_BUILD_STATIC` | `OFF` | 静态库构建 |
| `RMVPE_INFER_BUILD_STATIC` | `OFF` | 静态库构建 |
| `AUDIO_UTIL_BUILD_STATIC` | `OFF` | 静态库构建 |

### 7.3 构建命令
```bash
cmake -B build -G Ninja \
    -DCMAKE_PREFIX_PATH=<Qt6_DIR> \
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build --target all
cmake --build build --target install
```

### 7.4 CI/CD
- **触发**: 标签推送 (`v*`)
- **环境**: Windows, MSVC 2022, Qt 6.9.3, Ninja
- **产出**: `dataset-tools-msvc2022-x64-{date}-{hash}.zip`
- **发布**: 自动创建 GitHub Release

### 7.5 部署
- Windows: `windeployqt` 自动部署 Qt 依赖
- macOS: `macdeployqt` + 可选代码签名 (arm64)
- 所有 DLL/EXE 复制到安装目录

### 7.6 构建产出

| 类型 | 文件 |
|---|---|
| **应用** | `MinLabel.exe`, `SlurCutter.exe`, `AudioSlicer.exe`, `GameInfer.exe`, `HubertFA.exe`, `LyricFA.exe` |
| **测试** | `TestGame.exe`, `TestRmvpe.exe`, `TestSome.exe`, `TestAudioUtil.exe` |
| **库** | `game-infer.dll`, `rmvpe-infer.dll`, `some-infer.dll`, `audio-util.dll` |
| **依赖** | `onnxruntime.dll`, `SDL2.dll`, `sndfile.dll`, `soxr.dll`, `qBreakpad.dll`, 等 |

---

## 8. 非功能性需求

### 8.1 性能
- ONNX Runtime GPU 加速 (DirectML / CUDA) 用于推理密集型工具
- 所有批处理工具限制 QThreadPool maxThreadCount=1 (串行处理)
- 流式 RMS 计算 (MovingRMS) 避免全文件加载
- IntervalTree 空间索引用于 F0Widget 音符查询

### 8.2 可用性
- 拖放文件添加 (AudioSlicer, LyricFA, HubertFA)
- 可自定义快捷键 (MinLabel, SlurCutter)
- Windows 任务栏进度指示 (AudioSlicer via ITaskbarList3)
- 崩溃报告 (MinLabel via qBreakpad)

### 8.3 兼容性
- **输入音频**: WAV, MP3, M4A, FLAC
- **输出格式**: WAV (多种位深), MIDI, TextGrid, JSON, CSV, .lab, .ds
- **平台**: Windows (主要), macOS, Linux

### 8.4 安全性
- 检测并拒绝以 root/管理员身份运行 (MinLabel, SlurCutter)

---

## 9. 环境变量

| 变量 | 默认值 | 说明 | 使用位置 |
|---|---|---|---|
| `QT_DIR` / `Qt6_DIR` | — | Qt 安装路径 | 构建系统 |
| `VCPKG_KEEP_ENV_VARS` | — | 保留 Qt 路径于 vcpkg | 构建系统 |
| `QSAPI_AUDIO_DECODER` | `"FFmpegDecoder"` | 音频解码器插件 | `AudioDecoderFactory.cpp:93` |
| `QSAPI_AUDIO_PLAYBACK` | `"SDLPlayback"` | 音频播放器插件 | `AudioPlaybackFactory.cpp:93` |

---

## 10. 术语表

| 术语 | 说明 |
|---|---|
| DiffSinger | 基于扩散模型的歌声合成系统 |
| F0 | 基频 (Fundamental Frequency)，即音高 |
| G2P | Grapheme-to-Phoneme，字素到音素转换 |
| TextGrid | Praat 语音标注格式 |
| RMVPE | Robust Model for Vocal Pitch Estimation |
| SOME | Singing-Oriented MIDI Estimation |
| GAME | General Audio-to-MIDI Estimation |
| FunASR | Alibaba DAMO 语音识别框架 |
| Paraformer | FunASR 中的端到端 ASR 模型 |
| D3PM | Discrete Denoising Diffusion Probabilistic Model |
| Slur | 连音，多个音符绑定为同一音节 |
| Glide | 滑音装饰 (上滑/下滑) |
| AP | Aspiration Pause，呼吸音 |
| SP | Silent Pause，静音段 |
| DS | DiffSinger 句子格式 (.ds 文件) |
| DirectML | Microsoft DirectX Machine Learning API |
| CMVN | Cepstral Mean-Variance Normalization |
| LFR | Low Frame Rate (帧率降低) |
| PPQ | Pulses Per Quarter note (MIDI 时间精度) |
| IntervalTree | 区间树数据结构，用于空间查询 |
