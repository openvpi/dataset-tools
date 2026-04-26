# GUI 应用集成测试

本文档定义 6 个 GUI 应用的 CLI 级集成测试方案。由于应用为 Qt GUI 程序，测试分为：
- **可自动化**: 数据处理逻辑，通过构造输入文件 + 验证输出文件
- **手动验收**: UI 交互、音频播放、拖放操作

---

## 1. AudioSlicer

### AS-T001: 基本切片功能

**前置**: 准备包含明确静音段的 WAV 文件。

**自动化方案**: AudioSlicer 的核心逻辑在 `WorkThread::run()` 中，依赖 `AudioUtil::Slicer`。可通过 TestAudioUtil 中新增的 Slicer 测试覆盖核心算法。

**手动验收**:
1. 启动 AudioSlicer
2. 拖放 WAV 文件到文件列表
3. 设置参数: Threshold=-40, Min Length=5000, Min Interval=300
4. 点击 Start
5. 验证: 输出目录中切片文件数正确，无空文件

### AS-T002: CSV 标记导入/导出 (BUG-006 回归)

**关键**: 验证 `capturedView()` 索引修复。

**测试用例**:
```
# 构造 Audacity CSV
echo "0:00:01.500	0:00:03.200	slice_1" > /tmp/test_markers.csv
echo "0:00:05.000	0:00:07.800	slice_2" >> /tmp/test_markers.csv
```

**手动验收**:
1. File → Load CSV Markers
2. 选择上述 CSV
3. 验证: 标记正确显示 (1.5s-3.2s, 5.0s-7.8s)
4. 如果标记全部为 0 或报错 → BUG-006 未修复

### AS-T003: 输出位深验证

**手动步骤**:
1. 分别选择 16/24/32-bit PCM 和 32-bit float
2. 切片同一文件
3. 使用 `ffprobe` 验证输出:
```bash
ffprobe -i output_16bit.wav -show_entries stream=bits_per_raw_sample -hide_banner
```

---

## 2. MinLabel

### ML-T001: JSON 读写往返

**自动化方案**: 提取 `Common::readJsonFile` / `Common::writeJsonFile` 的逻辑做单元测试。

**手动验收**:
1. 打开含 WAV 的目录
2. 编辑 lab 和 raw_text 字段
3. 切换到另一文件 (触发自动保存)
4. 切换回来 → 验证内容保留

### ML-T002: G2P 转换验证

| 输入 | 语言 | 预期输出 |
|---|---|---|
| "你好" | 普通话 | "ni hao" (去调) 或 "ni3 hao3" (带调) |
| "世界" | 普通话 | "shi jie" |
| "こんにちは" | 日语 | "konnichiha" 或类似罗马字 |

**手动步骤**:
1. 在 raw_text 输入中文
2. 点击 G2P 转换
3. 验证 lab 字段输出

### ML-T003: Lab 转 JSON 批量

**手动步骤**:
1. 准备目录: 3 个 WAV + 3 个 .lab (无 .json)
2. File → Convert Lab to JSON
3. 验证: 3 个 .json 文件生成，内容含 lab 字段

---

## 3. SlurCutter

### SC-T001: DS 文件加载

**准备** `test.ds`:
```json
[
  {
    "text": "ni hao",
    "ph_seq": "n i h ao",
    "ph_dur": "0.1 0.2 0.1 0.3",
    "note_seq": "C4 D4",
    "note_dur": "0.3 0.4",
    "note_slur": "0 0",
    "f0_seq": "261.6 293.7",
    "f0_timestep": 0.005
  }
]
```

**手动验收**:
1. File → Open → 选择对应 WAV
2. 验证: F0 曲线显示，2 个音符可见
3. 验证: 句子列表显示 1 个句子

### SC-T002: 编辑 + 保存往返

1. 加载 DS 文件
2. 拖拽一个音符改变音高
3. 右键 → Split Note (分割)
4. Ctrl+S 保存
5. 重新打开 → 验证修改保留

### SC-T003: 空 F0 边界条件 (BUG-013)

**准备** `empty_f0.ds`:
```json
[{"text": "test", "ph_seq": "t", "ph_dur": "0.5", "note_seq": "C4", "note_dur": "0.5", "note_slur": "0", "f0_seq": "", "f0_timestep": 0.005}]
```

**预期**: 不崩溃，显示错误提示或空 F0 区域。

### SC-T004: 纯音符名解析 (BUG-024)

**准备** DS 文件中 `note_seq` 含无偏移音符名 `"C4"` (无 `+0` 或 `-0`)。

**预期**: `splitPitch[1]` 不越界，cents 默认为 0。

---

## 4. LyricFA

### LF-T001: ASR 识别

**前置**: AsrModel 已下载到 `model/ParaformerAsrModel/`

**手动步骤**:
1. 启动 LyricFA
2. 选择模型目录
3. 添加中文语音 WAV
4. 运行 Phase 1 (ASR)
5. 验证: .lab 文件生成，内容包含中文字符

### LF-T002: 歌词匹配

1. 运行 Phase 1 生成 .lab
2. 准备参考歌词文件
3. 运行 Phase 2 (匹配)
4. 验证: .json 生成，含 raw_text 和 lab

### LF-T003: 中文字符清洗 (BUG-005)

**验证正则**: `[^\u4e00-\u9fa5]` 应仅保留中文字符。

测试输入: `"Hello你好World世界123"`
预期输出: `"你好世界"`

### LF-T004: 静音/空音频

1. 输入纯静音 WAV
2. 运行 ASR
3. 预期: 不崩溃，输出空 .lab 或报错

---

## 5. HubertFA

### HF-T001: 模型配置验证

**前置**: HuBERT 模型目录 (config.json + model.onnx + vocab.json)

**手动步骤**:
1. 启动 HubertFA
2. 选择模型目录
3. 验证: 语言列表正确加载

### HF-T002: 对齐输出 TextGrid

1. 加载模型
2. 添加 WAV + 文本
3. 运行对齐
4. 验证: TextGrid 输出包含 phones tier 和 words tier

### HF-T003: 模型加载失败 (BUG-021)

1. 指向一个空目录或不完整模型
2. 运行对齐
3. 预期: 不崩溃，显示错误消息

---

## 6. GameInfer

### GI-T001: 端到端推理

**前置**: GAME 模型目录

**手动步骤**:
1. 启动 GameInfer
2. 选择模型目录、输入 WAV、输出 MIDI 路径
3. 点击导出
4. 验证: MIDI 文件生成，可用 MIDI 播放器打开

### GI-T002: 参数调整

1. 修改 seg_threshold、est_threshold、D3PM steps
2. 重新推理
3. 验证: 输出 MIDI 音符数/音高有变化

### GI-T003: 配置持久化

1. 设置各参数 → 关闭应用
2. 重新打开 → 验证参数恢复
3. 验证: `config/GameInfer.ini` 内容正确

---

## 7. 自动化测试脚本模板

以下 PowerShell 脚本可用于 CI 中的 smoke test：

```powershell
# smoke-test.ps1
param(
    [string]$BuildDir = "build/bin",
    [string]$TestDataDir = "test-data"
)

$ErrorCount = 0

function Test-Exe {
    param([string]$Name, [string]$Args, [int]$ExpectedExit = 0)
    $exe = Join-Path $BuildDir "$Name.exe"
    if (!(Test-Path $exe)) {
        Write-Host "SKIP: $Name not found" -ForegroundColor Yellow
        return
    }
    $process = Start-Process -FilePath $exe -ArgumentList $Args -Wait -PassThru -NoNewWindow 2>$null
    if ($process.ExitCode -ne $ExpectedExit) {
        Write-Host "FAIL: $Name (exit=$($process.ExitCode), expected=$ExpectedExit)" -ForegroundColor Red
        $script:ErrorCount++
    } else {
        Write-Host "PASS: $Name" -ForegroundColor Green
    }
}

# L1: Library tests
Test-Exe "TestAudioUtil" "$TestDataDir/audio/test_mono_44100.wav $env:TEMP/out.wav"

# 验证输出
if (!(Test-Path "$env:TEMP/out.wav")) {
    Write-Host "FAIL: TestAudioUtil output not found" -ForegroundColor Red
    $ErrorCount++
}

# 构建产出验证
$requiredExes = @("MinLabel", "SlurCutter", "AudioSlicer", "LyricFA", "HubertFA", "GameInfer")
foreach ($exe in $requiredExes) {
    $path = Join-Path $BuildDir "$exe.exe"
    if (Test-Path $path) {
        Write-Host "PASS: $exe.exe exists" -ForegroundColor Green
    } else {
        Write-Host "FAIL: $exe.exe missing" -ForegroundColor Red
        $ErrorCount++
    }
}

Write-Host "`nTotal failures: $ErrorCount"
exit $ErrorCount
```
