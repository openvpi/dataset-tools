# audio-util 库测试用例

**测试可执行文件**: `TestAudioUtil`
**源码**: `src/libs/audio-util/tests/main.cpp`
**当前覆盖**: 仅 WAV 重采样 + 写入

---

## 现有测试

### TC-AU-001: 基本重采样 + WAV 写入

```bash
TestAudioUtil <audio_in_path> <wav_out_path>
```

**步骤**:
1. `resample_to_vio()` 将输入重采样到 44100Hz mono
2. `SndfileHandle` 读取 VIO 数据
3. `write_vio_to_wav()` 写入输出 WAV

**验证**:
- 退出码 == 0
- 输出文件存在且大小 > 0
- 输出文件为有效 WAV (可用 `ffprobe` 验证)

**测试命令**:
```bash
# 正常用例
TestAudioUtil test-data/audio/test_mono_44100.wav /tmp/out_44100.wav
ffprobe -i /tmp/out_44100.wav -show_streams -hide_banner 2>&1 | grep "sample_rate=44100"

# 不同输入格式
TestAudioUtil test-data/audio/test.mp3 /tmp/out_mp3.wav
TestAudioUtil test-data/audio/test.flac /tmp/out_flac.wav

# 不同采样率
TestAudioUtil test-data/audio/test_stereo_48000.wav /tmp/out_48k.wav
```

---

## 建议新增测试

### TC-AU-002: 错误处理 — 不存在的输入文件

```bash
TestAudioUtil /nonexistent/path.wav /tmp/out.wav
```

**预期**: 退出码 != 0 或 stderr 有错误消息，不崩溃。

**实现建议** (修改 `main.cpp`):
```cpp
if (!std::filesystem::exists(audio_in_path)) {
    std::cerr << "Error: Input file does not exist: " << audio_in_path << std::endl;
    return 1;
}
```

### TC-AU-003: 错误处理 — 无效音频文件

```bash
# 创建一个假的 WAV (空文件或文本文件)
echo "not a wav file" > /tmp/fake.wav
TestAudioUtil /tmp/fake.wav /tmp/out.wav
```

**预期**: 不崩溃，返回错误。

### TC-AU-004: Slicer 单元测试 (需新建)

当前 Slicer 没有独立测试。建议在 `audio-util/tests/main.cpp` 中添加：

```cpp
// 测试 Slicer 基本功能
void test_slicer() {
    // 构造一段有已知静音段的音频
    // silence: [0, 1s], voice: [1s, 3s], silence: [3s, 4s], voice: [4s, 5s]
    const int sr = 44100;
    std::vector<float> audio(sr * 5, 0.0f);
    // 填充声音段
    for (int i = sr; i < sr * 3; i++) audio[i] = 0.5f * sin(2.0 * M_PI * 440.0 * i / sr);
    for (int i = sr * 4; i < sr * 5; i++) audio[i] = 0.5f * sin(2.0 * M_PI * 440.0 * i / sr);

    AudioUtil::Slicer slicer(-40.0, 300, 300, 10, 500);
    auto markers = slicer.slice(audio, sr);

    // 预期: 2 个切片 [~1s, ~3s] 和 [~4s, ~5s]
    assert(markers.size() == 2);
    std::cout << "Slicer test passed!" << std::endl;
}
```

### TC-AU-005: Slicer 边界条件

```cpp
void test_slicer_edge_cases() {
    AudioUtil::Slicer slicer(-40.0, 300, 300, 10, 500);

    // 空音频
    {
        std::vector<float> empty;
        auto markers = slicer.slice(empty, 44100);
        assert(markers.empty());
    }

    // 纯静音
    {
        std::vector<float> silence(44100 * 3, 0.0f);
        auto markers = slicer.slice(silence, 44100);
        assert(markers.empty()); // 无有效音频段
    }

    // 无静音 (连续声音)
    {
        std::vector<float> voice(44100 * 3);
        for (size_t i = 0; i < voice.size(); i++)
            voice[i] = 0.5f * sin(2.0 * M_PI * 440.0 * i / 44100);
        auto markers = slicer.slice(voice, 44100);
        assert(markers.size() == 1); // 整段作为一个切片
    }

    std::cout << "Slicer edge case tests passed!" << std::endl;
}
```

### TC-AU-006: 重采样精度验证

```cpp
void test_resample_accuracy() {
    // 生成已知频率正弦波 → 重采样 → FFT 验证频率保留
    // 或简单地验证: 输入样本数 / 输入采样率 ≈ 输出样本数 / 输出采样率
    const int in_sr = 48000;
    const int out_sr = 16000;
    const float duration = 2.0f;
    const size_t in_samples = in_sr * duration;
    const size_t expected_out_samples = out_sr * duration;

    // ... 调用 resample_to_vio 并验证输出帧数
    // 允许 ±1 帧误差
    assert(abs((int)actual_frames - (int)expected_out_samples) <= 1);
}
```
