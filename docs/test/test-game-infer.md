# game-infer 库测试用例

**测试可执行文件**: `TestGame`
**源码**: `src/libs/game-infer/tests/main.cpp`
**当前覆盖**: 完整端到端推理 (加载模型 → 推理 → 输出 MIDI)

---

## 现有测试

### TC-GI-001: 端到端音频转 MIDI

```bash
TestGame <audio_path> <model_dir> <output_dir> [options...]
```

**完整选项**:
```
--language <lang>           语言代码
--batch-size <int>          批量大小 (默认 1) [仅 Usage 显示，未解析]
--num-workers <int>         工作线程数 (默认 0) [仅 Usage 显示，未解析]
--seg-threshold <float>     分割阈值 (默认 0.2)
--seg-radius <float>        分割半径秒 (默认 0.02)
--seg-d3pm-t0 <float>      D3PM 起始 t (默认 0.0)
--seg-d3pm-nsteps <int>    D3PM 步数 (默认 8)
--seg-d3pm-ts <f,f,...>    自定义 D3PM 时间步 (覆盖 t0/nsteps)
--est-threshold <float>     估计阈值 (默认 0.2)
--input-formats <...>       输入格式 (默认 wav,flac,mp3,aac,ogg) [仅 Usage 显示，未解析]
--output-formats <...>      输出格式 (默认 mid) [仅 Usage 显示，未解析]
--tempo <float>             BPM (默认 120)
--pitch-format <number|name> 音高格式 (默认 name) [仅 Usage 显示，未解析]
--round-pitch               标志: 四舍五入音高
--provider <cpu|dml|cuda>   执行提供程序 (默认 cpu)
--device-id <int>           GPU ID (默认 0)
```

> **注意**: `--batch-size`、`--num-workers`、`--input-formats`、`--output-formats`、
> `--pitch-format` 出现在 Usage 消息中，但参数解析循环中未处理。

**测试命令**:
```bash
# 基本推理
TestGame test-data/audio/test_mono_44100.wav test-data/models/GAME-test /tmp/out

# 验证输出
ls /tmp/out/test_mono_44100.mid  # 文件应存在

# 不同参数
TestGame test-data/audio/test_mono_44100.wav test-data/models/GAME-test /tmp/out \
    --seg-threshold 0.3 --est-threshold 0.3 --tempo 90

# DirectML (有 GPU 时)
TestGame test-data/audio/test_mono_44100.wav test-data/models/GAME-test /tmp/out \
    --provider dml --device-id 0
```

---

## 已有测试中的已知问题

1. **`--round-pitch` 标志解析 bug**: 由于 `i += 2` 循环步进，`--round-pitch` (无参数标志) 的处理中 `i--` 调整可能在特定参数顺序下跳过下一个参数。此外 `main.cpp:140-146` 有无用的 dead code。

2. **语言映射硬编码**: `game.set_language(0)` 忽略实际语言参数，应从 config.json 的 languages map 查找。

3. **MIDI Track Name**: 输出 MIDI 的 Track 1 名称硬编码为 `"Game"` (`main.cpp:226`)。

---

## 建议新增测试

### TC-GI-002: 模型目录不完整

```bash
# 缺失 config.json
mkdir /tmp/bad_model
TestGame test-data/audio/test.wav /tmp/bad_model /tmp/out
# 预期: 退出码 1, stderr 含 "config.json not found"
```

**当前状态**: 已覆盖 (main.cpp:77-79)

### TC-GI-003: 音频文件不存在

```bash
TestGame /nonexistent.wav test-data/models/GAME-test /tmp/out
# 预期: 退出码 1, stderr 含 "does not exist"
```

**当前状态**: 已覆盖 (main.cpp:67-69)

### TC-GI-004: 空 / 超短音频

```bash
TestGame test-data/audio/empty.wav test-data/models/GAME-test /tmp/out
TestGame test-data/audio/silence_3s.wav test-data/models/GAME-test /tmp/out
```

**预期**: 不崩溃，可能输出空 MIDI 或报错。

**当前状态**: **未覆盖**，需验证 `game.get_midi()` 对 0 帧音频的处理。

### TC-GI-005: MIDI 输出验证

```bash
TestGame test-data/audio/test.wav test-data/models/GAME-test /tmp/out
# 使用外部工具验证 MIDI
python3 -c "
import mido
mid = mido.MidiFile('/tmp/out/test.mid')
print(f'Tracks: {len(mid.tracks)}')
print(f'Type: {mid.type}')
notes = [m for t in mid.tracks for m in t if m.type == 'note_on']
print(f'Notes: {len(notes)}')
assert mid.type == 1
assert len(mid.tracks) == 2

# 验证 Track Name 为 'Game'
track1 = mid.tracks[1]
meta_events = [e for e in track1 if e.type == 'track_name']
assert len(meta_events) == 1
assert meta_events[0].name == 'Game', f'Expected track name Game, got {meta_events[0].name}'
"
```

### TC-GI-006: D3PM 自定义时间步

```bash
TestGame test-data/audio/test.wav test-data/models/GAME-test /tmp/out \
    --seg-d3pm-ts 0.0,0.125,0.25,0.375,0.5,0.625,0.75,0.875
# 验证: 日志输出 "d3pm_ts count: 8"
```

### TC-GI-007: 参数组合测试

| 场景 | seg_threshold | est_threshold | d3pm_nsteps | 预期 |
|---|---|---|---|---|
| 默认 | 0.2 | 0.2 | 8 | 正常输出 |
| 宽松阈值 | 0.5 | 0.5 | 4 | 更少音符 |
| 严格阈值 | 0.05 | 0.05 | 16 | 更多音符 |
| 零步 | 0.2 | 0.2 | 0 | d3pmTs 为空，需验证不崩溃 |

### TC-GI-008: 未解析选项验证

以下选项出现在 Usage 中但实际未被解析，传入时应被静默忽略（不崩溃）：

```bash
TestGame test-data/audio/test_mono_44100.wav test-data/models/GAME-test /tmp/out \
    --batch-size 4 --num-workers 2 --input-formats wav,mp3 \
    --output-formats mid,csv --pitch-format number
# 预期: 正常运行（未知选项被跳过），输出 MIDI
```

**当前状态**: 由于 `i += 2` 循环步进，未识别的键值对选项实际上会被跳过，但这是隐式行为而非显式处理。

### TC-GI-009: `--round-pitch` 位置敏感性

```bash
# --round-pitch 在中间位置
TestGame test-data/audio/test_mono_44100.wav test-data/models/GAME-test /tmp/out \
    --seg-threshold 0.3 --round-pitch --est-threshold 0.3
# 预期: 所有参数正确解析，seg_threshold=0.3, est_threshold=0.3, roundPitch=true

# --round-pitch 在末尾
TestGame test-data/audio/test_mono_44100.wav test-data/models/GAME-test /tmp/out \
    --seg-threshold 0.3 --round-pitch
# 预期: seg_threshold=0.3, roundPitch=true
```

**当前状态**: **需验证**。`i--` 修正在 `--round-pitch` 位于参数中间时可能导致下一个选项的 key 被当作前一个的 val 消费。
