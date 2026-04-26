# 测试矩阵 — 全功能覆盖表

**目标**: 确保每个可测试功能至少有一个对应的测试项。

---

## 1. 库测试矩阵 (L1)

### audio-util

| ID | 功能 | 测试项 | 当前状态 | 优先级 |
|---|---|---|---|---|
| AU-001 | WAV 读取 (mono/stereo, 各采样率) | TestAudioUtil 基本流程 | 已有 | P0 |
| AU-002 | MP3 解码 (mpg123) | 输入 MP3 → 验证输出 WAV | **缺失** | P0 |
| AU-003 | FLAC 解码 | 输入 FLAC → 验证输出 WAV | **缺失** | P1 |
| AU-004 | 重采样 (soxr) | 44100→16000、48000→44100 | 部分覆盖 | P0 |
| AU-005 | SndfileVio 虚拟 I/O | 内存读写往返 | 部分覆盖 | P1 |
| AU-006 | Slicer 静音检测 | 已知静音位置的音频 → 验证切片点 | **缺失** | P0 |
| AU-007 | Slicer 边界条件 | 空音频、纯静音、无静音 | **缺失** | P0 |
| AU-008 | write_vio_to_wav | 各位深 (16/24/32-bit PCM, 32-float) | 部分覆盖 | P1 |
| AU-009 | 错误处理 | 不存在的文件路径、损坏的文件 | **缺失** | P0 |

### game-infer

| ID | 功能 | 测试项 | 当前状态 | 优先级 |
|---|---|---|---|---|
| GI-001 | 模型加载 | 正常加载 + is_open() == true | 已有 | P0 |
| GI-002 | 模型加载失败 | 缺失 config.json / onnx 文件 | 已有(部分) | P0 |
| GI-003 | get_midi 正常推理 | 音频 → MIDI 音符数 > 0 | 已有 | P0 |
| GI-004 | 参数设置 | seg_threshold / seg_radius / est_threshold | 已有 | P1 |
| GI-005 | D3PM 时间步 | t0_n_step_to_ts 生成 + 自定义 ts | 已有 | P1 |
| GI-006 | 执行提供程序切换 | CPU / DML / CUDA | 已有(CLI参数) | P1 |
| GI-007 | 空音频 / 超短音频 | 0 样本或 < 0.1s | **缺失** | P0 |
| GI-008 | MIDI 输出验证 | 输出文件可被 MIDI 解析器读取，Track Name == "Game" | **缺失** | P1 |
| GI-009 | 进度回调 | progressChanged 被正确调用 | 隐式覆盖 | P2 |
| GI-010 | 未解析 CLI 选项 | --batch-size 等 Usage-only 选项不导致崩溃 | **缺失** | P2 |
| GI-011 | --round-pitch 位置敏感性 | 不同参数顺序下 --round-pitch 正确解析 | **缺失** | P1 |

### some-infer

| ID | 功能 | 测试项 | 当前状态 | 优先级 |
|---|---|---|---|---|
| SI-001 | 模型加载 + 推理 | WAV → MIDI 输出 | 已有 | P0 |
| SI-002 | 执行提供程序 | CPU / DML | 已有(CLI参数) | P1 |
| SI-003 | tempo 参数 | 不同 tempo 值验证 MIDI 时间正确性 | **缺失** | P1 |
| SI-004 | 空音频 | 错误处理 | **缺失** | P0 |
| SI-005 | 错误消息 | 模型路径无效时 msg 非空 | **缺失** | P0 |

### rmvpe-infer

| ID | 功能 | 测试项 | 当前状态 | 优先级 |
|---|---|---|---|---|
| RI-001 | 模型加载 + F0 提取 | WAV → f0 + uv 数组 | 已有 | P0 |
| RI-002 | 异步推理 | std::async 正常完成 | 已有 | P1 |
| RI-003 | CSV 输出 | writeCsv 输出格式正确 | 已有(注释状态) | P2 |
| RI-004 | 空音频 | 错误处理 | **缺失** | P0 |
| RI-005 | threshold 参数 | 不同阈值对 uv 检测的影响 | **缺失** | P1 |
| RI-006 | 取消推理 | terminate 中断（已注释代码） | **缺失** | P2 |

### qsmedia (无独立测试)

| ID | 功能 | 测试项 | 当前状态 | 优先级 |
|---|---|---|---|---|
| QM-001 | 音频解码 | 通过 AudioSlicer / MinLabel 等应用隐式覆盖 | 隐式覆盖 | P2 |
| QM-002 | 虚拟 I/O | 内存音频读写 | **缺失** | P2 |

### ffmpegdecoder (无独立测试)

| ID | 功能 | 测试项 | 当前状态 | 优先级 |
|---|---|---|---|---|
| FD-001 | FFmpeg 音频解码 | 各格式 (MP3/FLAC/AAC/OGG) → PCM | **缺失** | P1 |
| FD-002 | 错误处理 | 无效文件 / 不支持的格式 | **缺失** | P1 |

### sdlplayback (无独立测试)

| ID | 功能 | 测试项 | 当前状态 | 优先级 |
|---|---|---|---|---|
| SP-001 | SDL2 音频播放初始化 | 通过 GUI 应用隐式覆盖 | 隐式覆盖 | P2 |
| SP-002 | 无音频设备 | 初始化失败不崩溃 (BUG-020) | **缺失** | P1 |

### FunAsr (无独立测试)

| ID | 功能 | 测试项 | 当前状态 | 优先级 |
|---|---|---|---|---|
| FA-001 | Paraformer ASR 推理 | 中文语音 → 文本 | 通过 LyricFA 隐式覆盖 | P1 |
| FA-002 | 模型加载失败 | 缺失 model.onnx / vocab.txt | **缺失** | P1 |
| FA-003 | 空音频 / 静音 | 不崩溃 | **缺失** | P1 |

### AsyncTaskWindow (无独立测试)

| ID | 功能 | 测试项 | 当前状态 | 优先级 |
|---|---|---|---|---|
| AT-001 | 异步任务执行 | 通过 GUI 应用隐式覆盖 | 隐式覆盖 | P2 |
| AT-002 | 任务取消 | 运行中取消不崩溃 | **缺失** | P2 |
| AT-003 | 进度回调 | 进度正确传递到 UI | 隐式覆盖 | P2 |

---

## 2. 应用集成测试矩阵 (L2)

### AudioSlicer

| ID | 功能 | 测试方法 | 优先级 |
|---|---|---|---|
| AS-T001 | 基本切片 | 预设参数 + 已知音频 → 验证切片数 | P0 |
| AS-T002 | CSV 标记导入 | 构造 CSV → 解析 → 验证时间戳 (BUG-006) | P0 |
| AS-T003 | CSV 标记导出 | 切片后导出 → 验证 CSV 格式 | P1 |
| AS-T004 | 各输出位深 | 16/24/32-PCM, 32-float → 验证 WAV header | P1 |
| AS-T005 | 参数边界 | threshold=0, minLength>总长度, hopSize=0 | P0 |

### MinLabel

| ID | 功能 | 测试方法 | 优先级 |
|---|---|---|---|
| ML-T001 | JSON 读写往返 | 写入 JSON → 读回 → 字段一致 | P0 |
| ML-T002 | Lab 文件生成 | 从 JSON → .lab → 验证内容 | P0 |
| ML-T003 | G2P 普通话 | "你好" → "ni hao" (去调) | P0 |
| ML-T004 | G2P 日语 | "こんにちは" → romaji | P1 |
| ML-T005 | Lab 转 JSON 批量 | 目录中多个 .lab → 验证 JSON 数量和内容 | P1 |
| ML-T006 | 进度计算 | N 个文件目录 → 标注 M 个 → 验证百分比 (BUG-008) | P2 |

### SlurCutter

| ID | 功能 | 测试方法 | 优先级 |
|---|---|---|---|
| SC-T001 | DS 文件加载 | 解析标准 DS → 验证句子数 | P0 |
| SC-T002 | DS 文件保存往返 | 加载 → 修改 → 保存 → 重新加载 → 验证 | P0 |
| SC-T003 | 空 F0 处理 | f0_seq 为空的 DS → 不崩溃 (BUG-013) | P0 |
| SC-T004 | 纯音符名解析 | "C4" (无 +/-) → 不越界 (BUG-024) | P0 |
| SC-T005 | 编辑追踪 | 编辑 → 验证 SlurEditedFiles.txt | P1 |

### LyricFA

| ID | 功能 | 测试方法 | 优先级 |
|---|---|---|---|
| LF-T001 | ASR 识别 | 中文语音 → .lab → 包含中文字符 | P0 |
| LF-T002 | 歌词匹配 | ASR lab + 参考歌词 → .json (raw_text + lab) | P0 |
| LF-T003 | 中文清洗 | 混合字符串 → 仅保留中文 (BUG-005) | P0 |
| LF-T004 | 空输入 | 静音音频 → 不崩溃 | P0 |

### HubertFA

| ID | 功能 | 测试方法 | 优先级 |
|---|---|---|---|
| HF-T001 | 模型加载验证 | config.json + model.onnx + vocab.json 验证 | P0 |
| HF-T002 | 对齐输出 | 音频 + 文本 → TextGrid → 验证 tier 结构 | P0 |
| HF-T003 | 空 session 处理 | 模型加载失败 → 不崩溃 (BUG-021) | P0 |
| HF-T004 | 多语言 G2P | 不同语言字典 → 正确音素 | P1 |

### GameInfer

| ID | 功能 | 测试方法 | 优先级 |
|---|---|---|---|
| GI-T001 | 端到端推理 | 音频 → MIDI → 文件存在且可解析 | P0 |
| GI-T002 | 配置持久化 | 设置参数 → 重启 → 验证 INI 值 | P1 |
| GI-T003 | GPU 枚举 | DML 模式下枚举 GPU → 列表非空 (有 GPU 时) | P2 |

---

## 3. 构建测试矩阵 (L3)

| ID | 测试项 | 验证方法 | 优先级 |
|---|---|---|---|
| BD-001 | CMake 配置成功 | `cmake -B build` 退出码 0 | P0 |
| BD-002 | 全量编译无 error | `cmake --build build` 无 error 输出 | P0 |
| BD-003 | 编译警告审查 | Warning 数不增加 (与 baseline 比较) | P1 |
| BD-004 | 所有 EXE 产出 | 6 个 app + 4 个 test 可执行文件存在 | P0 |
| BD-005 | 所有 DLL 产出 | 4 个 shared lib 存在 | P0 |
| BD-006 | 库测试禁用 | `*_BUILD_TESTS=OFF` 不编译测试目标 (注意: 顶层 `BUILD_TESTS` 不控制库测试) | P1 |
| BD-007 | DML=OFF 构建 | 不链接 DirectML | P2 |

---

## 4. 已知 Bug 回归测试

以下测试项专门覆盖 `docs/ISSUES.md` 中记录的 Bug，防止回归：

| Bug ID | 回归测试 | 对应测试项 |
|---|---|---|
| BUG-001 | PlayWidget 初始化失败不崩溃 | L4 手动 |
| BUG-003 | Glide 编辑后不 fall-through | SC-T002 |
| BUG-005 | 中文过滤正确 | LF-T003 |
| BUG-006 | CSV 时间戳解析正确 | AS-T002 |
| BUG-013 | 空 F0 不崩溃 | SC-T003 |
| BUG-015 | 模型路径不存在不崩溃 | LF-T004, AU-009 |
| BUG-016 | 短字符串不越界 | LF-T003 |
| BUG-020 | 无音频设备不崩溃 | L4 手动 |
| BUG-021 | 模型加载失败后推理不崩溃 | HF-T003 |
| BUG-024 | 纯音符名不越界 | SC-T004 |
