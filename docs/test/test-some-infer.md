# some-infer 库测试用例

**测试可执行文件**: `TestSome`
**源码**: `src/libs/some-infer/tests/main.cpp`
**当前覆盖**: 模型加载 + WAV → MIDI 推理

---

## 现有测试

### TC-SI-001: 端到端 MIDI 估计

```bash
TestSome <model_path> <wav_path> <dml/cpu> <device_id> <out_midi_path> <tempo:float>
```

**测试命令**:
```bash
# CPU 推理
TestSome test-data/models/some-test.onnx test-data/audio/test_mono_44100.wav cpu 0 /tmp/some_out.mid 120.0

# 验证
ls -la /tmp/some_out.mid  # 文件存在且大小 > 0

# DML 推理 (有 GPU 时)
TestSome test-data/models/some-test.onnx test-data/audio/test_mono_44100.wav dml 0 /tmp/some_dml.mid 120.0
```

**验证**:
- 退出码 == 0
- 输出 MIDI 文件存在
- stdout 输出 progress 百分比
- MIDI 文件格式: Format 1, 2 tracks, PPQ 480

---

## 建议新增测试

### TC-SI-002: 模型路径无效

```bash
TestSome /nonexistent/model.onnx test-data/audio/test.wav cpu 0 /tmp/out.mid 120
```

**预期**: 退出码 != 0，stderr 包含错误消息，不崩溃 (段错误)。

### TC-SI-003: 音频路径无效

```bash
TestSome test-data/models/some-test.onnx /nonexistent.wav cpu 0 /tmp/out.mid 120
```

**预期**: `success == false`，stderr 输出 "Error: ..."。

### TC-SI-004: Tempo 参数验证

```bash
# 不同 tempo
TestSome model.onnx test.wav cpu 0 /tmp/t60.mid 60.0
TestSome model.onnx test.wav cpu 0 /tmp/t180.mid 180.0

# 验证: t60 和 t180 的音符 tick 位置不同但对应相同时间
```

### TC-SI-005: 参数数量错误

```bash
# 参数不足 (需要 6 个)
TestSome model.onnx test.wav cpu 0
# 预期: 退出码 1，输出 Usage 消息
```

**当前状态**: 已覆盖 (main.cpp:14-17)

### TC-SI-006: 输出 MIDI 格式验证

```python
# 验证脚本
import mido
mid = mido.MidiFile('/tmp/some_out.mid')
assert mid.type == 1, f"Expected format 1, got {mid.type}"
assert len(mid.tracks) == 2, f"Expected 2 tracks, got {len(mid.tracks)}"
assert mid.ticks_per_beat == 480, f"Expected PPQ 480, got {mid.ticks_per_beat}"

# Track 0: 时间签名 + tempo
# Track 1: TrackName "Some" + note events
track1 = mid.tracks[1]
meta_events = [e for e in track1 if e.type == 'track_name']
assert len(meta_events) == 1
assert meta_events[0].name == "Some"
```
