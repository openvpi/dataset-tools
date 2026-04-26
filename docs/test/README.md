# DiffSinger Dataset Tools - Test Plan

**Version**: 1.0
**Date**: 2026-04-26

本文档定义 dataset-tools 项目的测试方案，覆盖 4 个已有测试可执行文件（TestAudioUtil、TestGame、TestSome、TestRmvpe）以及 6 个 GUI 应用的 CLI 级验收测试。

---

## 1. 测试架构概览

```
docs/test/
├── README.md                  # 本文件：总体测试方案
├── test-matrix.md             # 测试矩阵：全功能覆盖表
├── test-audio-util.md         # audio-util 库测试用例
├── test-game-infer.md         # game-infer 库测试用例
├── test-some-infer.md         # some-infer 库测试用例
├── test-rmvpe-infer.md        # rmvpe-infer 库测试用例
├── test-apps.md               # 6 个 GUI 应用的 CLI/集成测试
└── test-build.md              # 构建与 CI 验证测试
```

## 2. 测试分层

| 层级 | 范围 | 工具 | 自动化 |
|---|---|---|---|
| **L1: 库单元测试** | audio-util, game-infer, some-infer, rmvpe-infer | 现有 Test*.exe | 可 CI |
| **L2: 应用集成测试** | 6 个 GUI 应用的核心数据流 | 脚本驱动 + 文件比对 | 部分可 CI |
| **L3: 构建验证** | CMake 配置、编译、链接 | CI workflow | 全自动 |
| **L4: 手动验收** | UI 交互、播放、拖放 | 人工 | 否 |

## 3. 测试数据

测试数据存放于 `test-data/` 目录（不纳入 git，需手动准备或从 CI artifact 下载）：

```
test-data/
├── audio/
│   ├── test_mono_44100.wav       # 标准测试音频 (mono, 44100Hz, ~5s)
│   ├── test_stereo_48000.wav     # 立体声 (stereo, 48000Hz)
│   ├── test_16k_mono.wav         # 16kHz mono (ASR/RMVPE 用)
│   ├── test.mp3                  # MP3 格式
│   ├── test.flac                 # FLAC 格式
│   ├── silence_3s.wav            # 纯静音 (边界测试)
│   └── empty.wav                 # 0 样本 (错误处理测试)
├── models/
│   ├── GAME-test/                # GAME 模型目录 (config.json + *.onnx)
│   ├── some-test.onnx            # SOME 模型
│   ├── rmvpe-test.onnx           # RMVPE 模型
│   └── ParaformerAsrModel/       # FunASR 模型 (model.onnx + vocab.txt)
├── ds/
│   ├── test_sentence.ds          # 标准 DS 文件
│   └── empty_f0.ds               # f0_seq 为空的 DS 文件 (BUG-013)
├── lyrics/
│   ├── test_lyrics.txt           # 参考歌词文件
│   └── test.lab                  # ASR 输出 lab 文件
└── expected/
    ├── test_resampled.wav        # 重采样预期输出
    ├── test_sliced/              # 切片预期输出
    └── test.mid                  # MIDI 预期输出
```

## 4. 测试执行

### 快速验证 (Smoke Test)

```bash
# 构建 (库测试默认启用)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ...
cmake --build build

# L1: 库测试
./build/bin/TestAudioUtil test-data/audio/test_mono_44100.wav /tmp/out.wav
./build/bin/TestRmvpe test-data/models/rmvpe-test.onnx test-data/audio/test_16k_mono.wav cpu 0
./build/bin/TestSome test-data/models/some-test.onnx test-data/audio/test_mono_44100.wav cpu 0 /tmp/out.mid 120
./build/bin/TestGame test-data/audio/test_mono_44100.wav test-data/models/GAME-test /tmp/out --provider cpu

# L3: 构建验证
cmake --build build 2>&1 | grep -c "error" # 应为 0
```

## 5. 详细测试文档索引

- [test-matrix.md](test-matrix.md) — 全功能测试矩阵
- [test-audio-util.md](test-audio-util.md) — audio-util 库
- [test-game-infer.md](test-game-infer.md) — game-infer 库
- [test-some-infer.md](test-some-infer.md) — some-infer 库
- [test-rmvpe-infer.md](test-rmvpe-infer.md) — rmvpe-infer 库
- [test-apps.md](test-apps.md) — GUI 应用集成测试
- [test-build.md](test-build.md) — 构建验证
