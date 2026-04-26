# rmvpe-infer 库测试用例

**测试可执行文件**: `TestRmvpe`
**源码**: `src/libs/rmvpe-infer/tests/main.cpp`
**当前覆盖**: 模型加载 + F0 提取 (异步)

---

## 现有测试

### TC-RI-001: F0 基频提取

```bash
TestRmvpe <model_path> <wav_path> <dml/cpu> <device_id> [csv_output]
```

**测试命令**:
```bash
# 基本推理
TestRmvpe test-data/models/rmvpe-test.onnx test-data/audio/test_16k_mono.wav cpu 0

# 带 CSV 输出 (注意: CSV 输出代码当前被注释)
TestRmvpe test-data/models/rmvpe-test.onnx test-data/audio/test_16k_mono.wav cpu 0 /tmp/f0.csv

# DML
TestRmvpe test-data/models/rmvpe-test.onnx test-data/audio/test_16k_mono.wav dml 0
```

**验证**:
- 退出码 == 0
- stdout 输出 progress 百分比
- 推理不崩溃

---

## 当前代码注意事项

1. **CSV 输出被注释**: `main.cpp:70-85` 的 CSV 输出调用和结果验证代码全部注释掉了。`writeCsv` 函数本身（`main.cpp:12-28`）仍然存在且可用，但未被调用。返回类型已从 `f0 + uv` 改为 `RmvpeRes` 结构体，注释代码中仍引用旧的 `f0`/`uv` 变量，需适配新结构体才能恢复使用。

2. **异步执行**: 测试使用 `std::async` 启动推理，模拟实际使用场景。

3. **terminate 测试注释**: `main.cpp:65-66` 有注释掉的 terminate 超时测试。

---

## 建议新增测试

### TC-RI-002: 恢复 CSV 输出验证

```cpp
// 修复注释掉的代码，适配 RmvpeRes 结构
if (!res.empty()) {
    std::vector<float> f0;
    std::vector<bool> uv;
    for (const auto &r : res) {
        f0.push_back(r.f0);
        uv.push_back(r.uv);
    }

    if (!csvOutput.empty()) {
        writeCsv(csvOutput, f0, uv);
    }

    // 基本验证
    std::cout << "F0 frames: " << f0.size() << std::endl;
    assert(!f0.empty());
} else {
    std::cerr << "Error: empty result" << std::endl;
    return 1;
}
```

### TC-RI-003: 模型路径无效

```bash
TestRmvpe /nonexistent.onnx test-data/audio/test.wav cpu 0
```

**预期**: 不崩溃。构造函数中 ONNX Session 创建失败应被 catch。

### TC-RI-004: 空音频

```bash
TestRmvpe test-data/models/rmvpe-test.onnx test-data/audio/empty.wav cpu 0
```

**预期**: res 为空或推理失败，不崩溃。

### TC-RI-005: F0 值范围验证

```cpp
// F0 合理性检查
for (const auto &r : res) {
    if (!r.uv) {
        // 有声帧 F0 应在 50Hz ~ 2000Hz 范围内
        assert(r.f0 >= 50.0f && r.f0 <= 2000.0f);
    }
}
```

### TC-RI-006: CSV 格式验证

```bash
TestRmvpe model.onnx test.wav cpu 0 /tmp/f0.csv

# 验证 CSV 格式
head -5 /tmp/f0.csv
# 预期格式: <time_sec>,<f0_hz>,<uv_flag>
# 例: 0.00000,0.0,1
#     0.01000,261.63,0
```

### TC-RI-007: 参数数量错误

```bash
TestRmvpe model.onnx  # 参数不足
# 预期: 退出码 1，Usage 消息
```

**当前状态**: 已覆盖 (main.cpp:41-43, 检查 argc != 5 && argc != 6)
