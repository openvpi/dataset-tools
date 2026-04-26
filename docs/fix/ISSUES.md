# src/lib 模块问题清单

> 版本: 2.0 | 日期: 2026-04-26
> 范围: `src/libs/` 下所有模块
> 统计: **7 CRITICAL + 14 HIGH + 20 MEDIUM + 12 LOW = 53 个问题**

---

## 模块概览

| 模块 | 功能 | 主要依赖 |
|------|------|----------|
| audio-util | 音频 I/O、重采样、切片、格式解码 | libsndfile, soxr, mpg123 |
| qsmedia | Qt 音频媒体抽象层 (NAudio 移植) | Qt |
| ffmpegdecoder | 基于 FFmpeg 的音频解码器 | qsmedia, FFmpeg |
| sdlplayback | 基于 SDL2 的音频播放 | qsmedia, SDL2 |
| some-infer | SOME 旋律 ONNX 推理 | audio-util, onnxruntime, wolf-midi |
| rmvpe-infer | RMVPE F0 估计 ONNX 推理 | audio-util, onnxruntime |
| game-infer | GAME 音符转录 ONNX 推理 | audio-util, onnxruntime, nlohmann_json |
| FunAsr | Paraformer 语音识别 ONNX 推理 | onnxruntime, fftw3 |
| AsyncTaskWindow | 批量异步任务 Qt 窗口基类 | Qt |

---

## CRITICAL - 严重问题 (7)

### C-01: FunAsr 空指针解引用 - loadparams
- **文件**: `FunAsr/src/util.cpp:10-11`
- **类别**: 内存安全
- **描述**: `loadparams()` 调用 `fopen()` 后未检查返回值, 直接对可能为 NULL 的 `fp` 调用 `fseek`/`ftell`/`fread`, 会导致崩溃。
- **影响**: 文件不存在或权限不足时程序崩溃。

### C-02: FunAsr 空指针解引用 - SaveDataFile
- **文件**: `FunAsr/src/util.cpp:34-38`
- **类别**: 内存安全
- **描述**: `SaveDataFile()` 同样未检查 `fopen()` 返回值。
- **影响**: 同 C-01。

### C-03: FunAsr 缓冲区溢出 - SpeechWrap
- **文件**: `FunAsr/src/SpeechWrap.cpp:28-30`
- **类别**: 内存安全
- **描述**: `cache` 固定为 400 个 float。`update()` 将 `cache_size = total_size - offset` 个 float 拷贝进去, 无边界检查。若剩余数据 > 400 则溢出。
- **影响**: 堆内存破坏, 可能导致任意代码执行。

### C-04: FunAsr 内存分配未检查 - Audio::loadwav
- **文件**: `FunAsr/src/Audio.cpp:89-90`
- **类别**: 内存安全
- **描述**: `speech_data` 通过 `malloc` 分配后未检查是否为 NULL, 直接 `memset`/`memcpy`。
- **影响**: 内存不足时崩溃。

### C-05: SDLPlayback 缓冲区过度分配
- **文件**: `sdlplayback/private/SDLPlayback_p.cpp:72`
- **类别**: 内存安全 / 逻辑
- **描述**: `new float[pcm_buffer_size * sizeof(float)]` 分配了 `N*4` 个 float (因为 `sizeof(float)==4`), 实际只需 `N` 个。虽不会崩溃但浪费 4 倍内存, 且语义错误可能掩盖其他 bug。
- **影响**: 4 倍内存浪费; 如果后续代码以字节计算则可能出现越界。

### C-06: FunAsr loadparams aligned_malloc 未检查 *(新增)*
- **文件**: `FunAsr/src/util.cpp:15`
- **类别**: 内存安全
- **描述**: `aligned_malloc` 返回值可能为 nullptr, 直接传入 `fread` 使用, 无 NULL 检查。
- **影响**: 内存不足时 `fread` 写入 NULL 地址导致崩溃。

### C-07: SpeechWrap 悬空指针 - operator[] *(新增)*
- **文件**: `FunAsr/src/SpeechWrap.cpp:33-34`
- **类别**: 内存安全
- **描述**: `in` 是通过 `load()` 设置的原始指针, 无所有权管理。若调用方的缓冲区被释放或移动, `operator[]` 访问悬空指针, 属于 use-after-free。
- **影响**: 未定义行为, 可能导致数据损坏或崩溃。

---

## HIGH - 高优先级问题 (14)

### H-01: SDLPlayback CPU 忙等
- **文件**: `sdlplayback/private/SDLPlayback_p.cpp:117-118, 123-124`
- **类别**: 线程安全 / 性能
- **描述**: `play()` 和 `stop()` 中使用 `while (state == ...) {}` 空循环等待状态变更, 无 sleep/yield, 100% CPU 占用。且 `state` 仅为 `volatile int`, 在 C++ 中不具备同步语义。
- **影响**: 播放期间烧 CPU, 存在 UB。

### H-02: audio-util 大量代码重复
- **文件**: `audio-util/src/Util.cpp:300-345`
- **类别**: 可维护性
- **描述**: flush 循环中的声道转换逻辑与主循环完全重复 (~45 行)。
- **影响**: 维护困难, 修改一处漏改另一处。

### H-03: Mp3Decoder 线程安全问题
- **文件**: `audio-util/src/Mp3Decoder.cpp:13`
- **类别**: 线程安全
- **描述**: `mpg123_init()`/`mpg123_exit()` 是全局状态操作, 每次解码文件都调用, 非线程安全。
- **影响**: 多线程并发解码 MP3 时可能崩溃。

### H-04: Mp3Decoder 帧数计算错误
- **文件**: `audio-util/src/Mp3Decoder.cpp:109`
- **类别**: 逻辑
- **描述**: `outBuf.writef(pcm_buffer.data(), num_samples)` 中 `writef` 期望帧数, 但 `num_samples` 是总采样数。多声道时帧数应为 `num_samples / channels`。
- **影响**: 多声道 MP3 文件解码数据错误。

### H-05: volatile 误用于线程同步
- **文件**: `qsmedia/Api/private/IAudioPlayback_p.h:28`
- **类别**: 线程安全
- **描述**: `volatile int state` 用于线程间状态同步。C++ 中 `volatile` 不保证原子性和内存序。
- **影响**: 多线程下存在数据竞争, 行为未定义。

### H-06: FunAsr 使用已弃用的 C++17 API
- **文件**: `FunAsr/src/commonfunc.h:17`
- **类别**: 兼容性
- **描述**: 使用 `std::codecvt_byname` 和 `std::wstring_convert`, 在 C++17 中已弃用。
- **影响**: 新编译器可能警告或移除支持。

### H-07: GameModel 静态局部变量线程安全 + 缓存问题
- **文件**: `game-infer/src/GameModel.cpp:469-477`
- **类别**: 线程安全 / 逻辑
- **描述**: `static std::vector<std::string> segInputNames` 惰性初始化无同步保护。且该变量缓存了首次加载模型的 input names, 若后续加载不同模型文件, 将永远使用错误的缓存值。
- **影响**: 多线程数据竞争; 多模型场景下推理结果错误。

### H-08: FFmpegDecoder goto 控制流
- **文件**: `ffmpegdecoder/private/FFmpegDecoder_p.cpp:315-316`
- **类别**: 可维护性
- **描述**: 复杂解码循环使用 `goto` 标签进行错误处理, 控制流难以追踪。
- **影响**: 维护困难, 容易引入资源泄漏。

### H-09: FunAsr 全模块原始指针管理
- **文件**: FunAsr 全模块
- **类别**: 内存安全
- **描述**: 大量使用 `new`/`delete`、`malloc`/`free` 手动管理内存, 无 RAII。`FeatureQueue` push `new Tensor` 可能永远不被 pop。
- **影响**: 错误路径上的内存泄漏。

### H-10: FFmpegDecoder initDecoder 多处资源泄漏 *(新增)*
- **文件**: `ffmpegdecoder/private/FFmpegDecoder_p.cpp:50-160`
- **类别**: 内存安全
- **描述**: `initDecoder()` 中 `_formatContext` 分配后, 后续多个错误检查点 (lines 51, 58, 69, 80, 90, 121, 136, 147, 157) 均直接 `return false` 而未调用 `quitDecoder()` 释放已分配资源。
- **影响**: 打开异常音频文件时泄漏 FFmpeg 上下文。

### H-11: SDLPlayback workCallback 异常不安全的 mutex 用法 *(新增)*
- **文件**: `sdlplayback/private/SDLPlayback_p.cpp:222-244`
- **类别**: 线程安全
- **描述**: SDL 音频回调中使用 `mutex.lock()` / `mutex.unlock()` 而非 `std::lock_guard`。若回调中抛出异常, mutex 永远不会释放。
- **影响**: 死锁。

### H-12: FunAsr FeatureQueue 析构函数泄漏队列元素 *(新增)*
- **文件**: `FunAsr/src/FeatureQueue.cpp:12-13`
- **类别**: 内存安全
- **描述**: 析构函数仅 delete `buff`, 不会排空 `feature_queue` 并释放其中的 `Tensor<float>*` 指针。
- **影响**: 每次 FeatureQueue 销毁时泄漏所有未消费的 Tensor。

### H-13: FunAsr Audio 析构函数不排空 frame_queue *(新增)*
- **文件**: `FunAsr/src/Audio.cpp:36-43`, `FunAsr/include/Audio.h:37`
- **类别**: 内存安全
- **描述**: `frame_queue` 存储原始 `AudioFrame*` 指针。若 `fetch()` 未被调用, 这些指针在 Audio 析构时泄漏。
- **影响**: 内存泄漏。

### H-14: paraformer_onnx 悬空指针 - m_szInputNames *(新增)*
- **文件**: `FunAsr/src/paraformer_onnx.cpp:30-40`
- **类别**: 内存安全
- **描述**: `m_strInputNames.emplace_back()` 后, `m_szInputNames.push_back(item.c_str())` 存储 `const char*`。但后续 `m_strInputNames` 的 `push_back` 可能触发 vector 重分配, 导致之前存储的所有 `c_str()` 指针悬空。
- **影响**: 后续使用 `m_szInputNames` 中的指针为 UB, 可能导致推理崩溃或结果错误。

---

## MEDIUM - 中等优先级问题 (20)

### M-01: 命名空间注释错误
- **文件**: `audio-util/src/SndfileVio.cpp:53`
- **描述**: 闭合注释写为 `// namespace Rmvpe`, 实际应为 `// namespace AudioUtil`。

### M-02: Include guard 不匹配
- **文件**: `audio-util/src/MathUtils.h:18`
- **描述**: `#endif // LYRICFA_MATHUTILS_H` 与 `#ifndef MATHUTILS_H` 不匹配。

### M-03: 硬编码中文错误消息
- **文件**: `audio-util/src/FlacDecoder.cpp:28`
- **描述**: `"不支持的 FLAC 文件位深"` 与其他模块英文消息风格不一致。

### M-04: 推理模块大量代码重复
- **文件**: `some-infer/SomeModel.cpp`, `rmvpe-infer/RmvpeModel.cpp`, `game-infer/GameModel.cpp`
- **描述**: `initDirectML`、`initCUDA`、`cumulativeSum`、`calculateNoteTicks`、`calculateSumOfDifferences` 等函数在 3 个模块中完全重复 (~120+ 行)。

### M-05: Rmvpe 成员变量可见性
- **文件**: `rmvpe-infer/include/rmvpe-infer/Rmvpe.h:33`
- **描述**: `std::unique_ptr<RmvpeModel> m_rmvpe` 声明为 `public`, 应为 `private`。

### M-06: 未使用的结构体
- **文件**: `some-infer/include/some-infer/Some.h:19-21`
- **描述**: `Dur` 结构体声明后从未使用。

### M-07: CMakeLists glob 模式错误
- **文件**: `FunAsr/src/CMakeLists.txt:4`
- **描述**: `target_include_directories(${PROJECT_NAME} PUBLIC *.h)` 使用 glob 模式作为路径, 无效 CMake。

### M-08: 硬编码 onnxruntime 路径
- **文件**: `FunAsr/src/CMakeLists.txt:12-13, 19-26`
- **描述**: `../../onnxruntime/include` 及 `/usr/local/opt/fftw/lib` 等硬编码路径, 不可移植。

### M-09: WaveStream 采样数计算
- **文件**: `qsmedia/NAudio/WaveStream.cpp:61`
- **描述**: `SkipSamples`/`TotalSamples` 等计算未考虑声道数。

### M-10: FFmpegDecoder 使用已弃用 Qt API
- **文件**: `ffmpegdecoder/FFmpegDecoder.cpp:13`
- **描述**: `var.type()` 搭配 `QVariant::Int` 在 Qt6 中已弃用。

### M-11: 错误输出到 stdout
- **文件**: `audio-util/src/Util.cpp:118`
- **描述**: 错误信息使用 `std::cout` 而非 `std::cerr`。

### M-12: FunAsr 不安全字符运算
- **文件**: `FunAsr/src/Vocab.cpp:112, 123`
- **描述**: `word[0] = word[0] - 32` 假设 ASCII 小写, 对非 ASCII 或非小写字符会产生错误结果。

### M-13: FunAsr 类型双关未定义行为
- **文件**: `FunAsr/src/FeatureExtract.cpp:109-111`
- **描述**: `int val = 0x34000000; float min_resol = *((float*)&val);` 通过指针强转实现类型双关, 属于 UB。

### M-14: onnxruntime 链接可见性
- **文件**: `game-infer/CMakeLists.txt:88`
- **描述**: `target_link_libraries(... PUBLIC onnxruntime)` 将 onnxruntime 泄露给消费者, 应为 PRIVATE。

### M-15: SndfileVio qvio_read 无符号下溢 *(新增)*
- **文件**: `audio-util/src/SndfileVio.cpp:32`
- **类别**: 逻辑
- **描述**: `remainingBytes = byteArray.size() - seek`, 两者均为无符号类型。若 `seek > size()`, 减法结果回绕为极大正数, 后续 `bytesToRead` 使用该值读取远超缓冲区的数据。
- **影响**: 越界读取。

### M-16: SndfileVio qvio_seek 未做边界检查 *(新增)*
- **文件**: `audio-util/src/SndfileVio.cpp:12-28`
- **类别**: 逻辑
- **描述**: `SEEK_END` 搭配正 offset 会将 `seek` 设置为超出末尾的位置; 其他模式也未 clamp 到有效范围。
- **影响**: 后续 read/write 可能越界。

### M-17: SndfileVio qvio_write 未更新 seek *(新增)*
- **文件**: `audio-util/src/SndfileVio.cpp:41-47`
- **类别**: 逻辑
- **描述**: write 操作向 `byteArray` 末尾追加数据, 但未更新 `qvio->seek` 位置。后续 read 仍从旧位置读取。
- **影响**: read/write 交替使用时数据不一致。

### M-18: SF_VIO::info 未初始化 *(新增)*
- **文件**: `audio-util/include/audio-util/SndfileVio.h:25`
- **类别**: 内存安全
- **描述**: `SF_INFO info` 是 C 结构体, 无默认初始化。成员 `frames`, `samplerate`, `channels`, `format` 包含垃圾值。
- **影响**: 使用未初始化的 info 字段导致未定义行为。

### M-19: FunAsr Vocab::vector2stringV2 越界访问 *(新增)*
- **文件**: `FunAsr/src/Vocab.cpp:74`
- **类别**: 内存安全
- **描述**: `std::string word = vocab[*it]` 未检查 `*it` 是否在 `vocab.size()` 范围内。
- **影响**: 模型输出异常 token ID 时越界访问。

### M-20: FunAsr log_softmax 数值不稳定 *(新增)*
- **文件**: `FunAsr/src/util.cpp:143-155`
- **类别**: 逻辑
- **描述**: 与 `softmax` 不同, `log_softmax` 在调用 `exp()` 前未减去最大值。对于大输入值, `exp(din[i])` 会溢出为 infinity。
- **影响**: 推理结果包含 NaN/Inf, 识别精度下降。

### M-21: FFmpegDecoder extractInt 截断 *(新增)*
- **文件**: `ffmpegdecoder/FFmpegDecoder.cpp:21-27`
- **类别**: 逻辑
- **描述**: `res = var.toLongLong()` / `var.toULongLong()` / `var.toDouble()` 赋值给 `int`, 大值被静默截断。
- **影响**: 处理大文件或高采样率时参数错误。

### M-22: FFmpegDecoder decode 写入 NULL 缓冲区 *(新增)*
- **文件**: `ffmpegdecoder/private/FFmpegDecoder_p.cpp:249`
- **类别**: 内存安全
- **描述**: `memcpy(buf, ...)` 中 `buf` 可能为 NULL (skip 操作传入 `nullptr`)。若 cache 有数据, 仍执行 memcpy 到 NULL 地址。
- **影响**: skip 操作时可能崩溃。

### M-23: game-infer reinterpret_cast<bool*> UB *(新增)*
- **文件**: `game-infer/src/GameModel.cpp:373, 439, 444, 449, 540, 544, 627, 637, 647`
- **类别**: 逻辑 / UB
- **描述**: `reinterpret_cast<bool*>(uint8_t*)` 将 uint8_t 数据当作 bool 读取。C++ 标准中 bool 只能是 0 或 1, 其他值为 UB。
- **影响**: 编译器优化可能产生意外行为。

### M-24: game-infer wstring 跨平台问题 *(新增)*
- **文件**: `game-infer/src/GameModel.cpp:145`
- **类别**: 兼容性
- **描述**: `std::ifstream configFile(path.wstring())` 在 Linux 上行为不正确, 应使用 `path.string()` 或平台条件判断。
- **影响**: Linux 编译/运行时可能无法打开配置文件。

---

## LOW - 低优先级问题 (12)

### L-01: Provider 枚举重复
- **文件**: `some-infer/Provider.h`, `rmvpe-infer/Provider.h`, `game-infer/` 内部
- **描述**: `ExecutionProvider` 枚举在三个命名空间中完全重复。

### L-02: UI 硬编码中文无 i18n
- **文件**: `AsyncTaskWindow.cpp`
- **描述**: "添加文件"、"运行任务" 等 UI 字符串硬编码, 未使用 `tr()`。

### L-03: AsyncTaskWindow 仅支持 WAV
- **文件**: `AsyncTaskWindow.cpp:58`
- **描述**: `addFiles` 文件过滤器仅接受 `.wav`, 但推理模块支持 mp3/flac。

### L-04: 调试输出残留
- **文件**: `audio-util/src/Util.cpp:449` 等多处
- **描述**: 生产代码中残留 `std::cout << "Successfully wrote..."` 等调试打印。

### L-05: 死代码 - tmp.h
- **文件**: `FunAsr/src/tmp.h`
- **描述**: 整个文件定义的 `WenetParams` 结构体从未被引用。

### L-06: 宏在命名空间内
- **文件**: `FunAsr/include/ComDefine.h:5-9`
- **描述**: `#define S_BEGIN 0` 等宏定义在 `namespace FunAsr` 内部, 宏不受命名空间限制, 具有误导性。

### L-07: 注释掉的大段代码
- **文件**: `rmvpe-infer/tests/main.cpp:70-85`
- **描述**: 大段注释代码未清理。

### L-08: IAudioEncoder 空壳
- **文件**: `qsmedia/Api/IAudioEncoder`
- **描述**: 仅有构造/析构函数, 无任何方法, 功能未实现。

### L-09: FunAsr Tensor 违反 Rule of Five *(新增)*
- **文件**: `FunAsr/src/Tensor.h:48-52`
- **类别**: 设计
- **描述**: `Tensor` 使用 `aligned_malloc`/`aligned_free` 管理内存, 但未定义 copy constructor、move constructor、copy assignment、move assignment。若被意外拷贝, 会 double-free。
- **影响**: 隐含的 double-free 风险。

### L-10: aligned_malloc 对齐参数无验证 *(新增)*
- **文件**: `FunAsr/src/alignedmem.cpp:6`
- **类别**: 逻辑
- **描述**: `~(alignment - 1)` 掩码仅在 alignment 为 2 的幂时正确。函数未验证 alignment 参数。
- **影响**: 非 2 的幂对齐值导致未对齐的内存。

### L-11: cumulativeSum 空输入崩溃 *(新增)*
- **文件**: `some-infer/src/Some.cpp:26-27`, `game-infer/src/Game.cpp:27-28`
- **类别**: 逻辑
- **描述**: `cumsum[0] = durations[0]` 在 `durations` 为空时越界访问。`calculateNoteTicks` 同理。
- **影响**: 空 MIDI 输入时崩溃。

### L-12: #if vs #ifdef CUDA 宏不一致 *(新增)*
- **文件**: `some-infer/src/SomeModel.cpp:37` vs `rmvpe-infer/src/RmvpeModel.cpp:38`
- **类别**: 构建
- **描述**: `#if ONNXRUNTIME_ENABLE_CUDA` (未定义时求值为 0) 与 `#ifdef` 混用, 可能导致 CUDA 功能静默启用或禁用。
- **影响**: 构建配置不一致。

### L-13: Ort::MemoryInfo::CreateCpu 重复创建 *(新增)*
- **文件**: `game-infer/src/GameModel.cpp:310, 365, 420, 536, 614, 700`
- **类别**: 性能
- **描述**: 每次推理调用都 `Ort::MemoryInfo::CreateCpu(...)`, 可作为类成员创建一次。
- **影响**: 不必要的开销 (微小但累积)。

### L-14: FunAsr Vocab wchar_t 构造函数未处理 CRLF *(新增)*
- **文件**: `FunAsr/src/Vocab.cpp:27-38`
- **类别**: 逻辑
- **描述**: Windows `wchar_t` 版本的构造函数不像 `char` 版本那样处理 `\r\n` 换行。
- **影响**: Windows 上词汇表加载可能包含多余的 `\r` 字符。

### L-15: Slicer 有符号/无符号比较 *(新增)*
- **文件**: `audio-util/src/Slicer.cpp:126`
- **类别**: 类型安全
- **描述**: `int64_t i` 与 `rms_list.size()` (size_t) 比较, 有符号/无符号不匹配。
- **影响**: 极大列表时可能产生错误比较结果。

### L-16: MathUtils divIntRound 除零 + 溢出 *(新增)*
- **文件**: `audio-util/src/MathUtils.h:8-16`
- **类别**: 逻辑
- **描述**: (1) `d == 0` 时除零未定义行为。(2) `n + (d / 2)` 对大值可能溢出。
- **影响**: 特殊输入时崩溃或结果错误。

### L-17: FunAsr softmax/log_softmax malloc 未检查 *(新增)*
- **文件**: `FunAsr/src/util.cpp:121, 144`
- **类别**: 内存安全
- **描述**: `malloc` 返回值未检查 NULL。
- **影响**: 内存不足时崩溃。

### L-18: SDLPlayback devices().front() 空列表崩溃 *(新增)*
- **文件**: `sdlplayback/private/SDLPlayback_p.cpp:107-108`
- **类别**: 逻辑
- **描述**: 若系统无可用音频设备, `devices.front()` 在空 vector 上调用为 UB。
- **影响**: 无音频设备环境下崩溃。
