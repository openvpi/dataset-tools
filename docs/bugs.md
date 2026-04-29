# 未修复 Bug 清单

**Version**: 1.2 | **Date**: 2026-04-29 | **基准**: 重构后代码

> 本文档列出所有**未修复**的 Bug。

---

## Critical (5)

### C-03: FunAsr SpeechWrap 缓冲区溢出 [Lib]

- **位置**: `src/infer/FunAsr/src/SpeechWrap.cpp`
- **描述**: SpeechWrap 在处理音频数据时，写入缓冲区未做边界检查，输入超出预期长度时会越界写入。
- **影响**: 内存损坏，可能被恶意音频文件利用。
- **建议修复**: 在写入前检查剩余缓冲区容量，超出时截断或返回错误。
- **工时**: S（半天）

### C-04: FunAsr malloc 返回值未检查 [Lib]

- **位置**: `src/infer/FunAsr/src/Audio.cpp`
- **描述**: `malloc` 分配内存后未检查返回值是否为 NULL，内存不足时直接对空指针操作。
- **影响**: 空指针解引用导致崩溃。
- **建议修复**: 检查 malloc 返回值，失败时清理并返回错误。
- **工时**: XS（15分钟）

### C-06: FunAsr aligned_malloc 返回值未检查 [Lib]

- **位置**: `src/infer/FunAsr/src/util.cpp`
- **描述**: 对齐内存分配后未检查返回值。
- **影响**: 同 C-04，空指针解引用。
- **建议修复**: 添加 NULL 检查。
- **工时**: XS（15分钟）

### C-07: FunAsr SpeechWrap 悬空指针 [Lib]

- **位置**: `src/infer/FunAsr/src/SpeechWrap.cpp`
- **描述**: 对象释放后指针未置空，后续代码可能访问已释放的内存。
- **影响**: Use-after-free，行为未定义。
- **建议修复**: 释放后置空指针，或改用 `std::unique_ptr`。
- **工时**: S（半天）

### C-05: SDLPlayback pcm_buffer 过度分配 [Lib]

- **位置**: `src/audio/`（SDLPlayback 相关代码）
- **描述**: `pcm_buffer.reset(new float[pcm_buffer_size * sizeof(float)])` 多乘了一个 `sizeof(float)`，实际分配了 4 倍于所需的内存。
- **影响**: 内存浪费 4 倍。虽不会崩溃，但属于明确的逻辑错误。
- **建议修复**: 改为 `pcm_buffer.reset(new float[pcm_buffer_size])`。
- **工时**: XS（5分钟）
- **别名**: CQ-011

---

## High (14)

### BUG-007: HubertFA 内存泄漏（3 个裸指针） [App]

- **位置**: `src/apps/pipeline/hubertfa/`（hubert-infer 尚未提取，代码仍在应用层）
- **描述**: `m_dictG2p`、`m_alignmentDecoder`、`m_nonLexicalDecoder` 三个对象通过裸 `new` 创建，析构函数中未释放。
- **影响**: 每次 HubertFA 运行结束泄漏三块内存。长时间运行或批量处理时累积。
- **建议修复**: 改用 `std::unique_ptr`。
- **工时**: S（半天）

### BUG-018: volatile 误用 + 忙等待 [App]

- **位置**: `src/audio/`（AudioPlayback 内部实现）
- **描述**: 用 `volatile int` 做线程间状态同步，配合 `while(state == Stopped) {}` 忙等待循环。`volatile` 在 C++ 中不保证原子性，也不提供内存栅栏。忙等待则浪费 100% CPU。
- **影响**: 潜在的数据竞争（未定义行为）+ CPU 空转。
- **建议修复**: `volatile` → `std::atomic<int>` + `std::condition_variable` 替代忙等待。
- **工时**: M（1~2天，需审查 SDL 回调线程模型）
- **别名**: H-01, H-05

### BUG-019: SDL mutex 无 RAII 保护 [App]

- **位置**: `src/audio/`（SDLPlayback 相关代码）
- **描述**: 手动调用 `mutex.lock()` / `mutex.unlock()`，异常或提前 return 时不会自动解锁。
- **影响**: 死锁风险。
- **建议修复**: 改用 `std::lock_guard` 或 `std::unique_lock`。
- **工时**: S（半天，需确认回调内无遗漏分支）
- **别名**: H-11

### BUG-020: SDL devices.front() 空列表崩溃 [App]

- **位置**: `src/audio/`（SDLPlayback 设备枚举）
- **描述**: 获取音频设备列表后直接调用 `devices.front()`，未检查列表是否为空。无音频设备时崩溃。
- **影响**: 无音频设备环境下必现崩溃。
- **建议修复**: 添加 `if (!devices.isEmpty())` 前置检查。
- **工时**: XS（15分钟）

### BUG-021: HfaModel 空 session 未检查 [App]

- **位置**: `src/apps/pipeline/hubertfa/` 中的 HfaModel.cpp（hubert-infer 未提取前）
- **描述**: `forward()` 方法未检查 `m_model_session` 是否为空，模型加载失败后调用推理会空指针崩溃。
- **影响**: 模型文件缺失或加载失败时崩溃。
- **建议修复**: 在 `forward()` 入口检查 `m_model_session`，为空时返回错误。
- **工时**: XS（15分钟）

### BUG-022: FunAsr ModelImp 资源泄漏 [App]

- **位置**: `src/infer/FunAsr/src/paraformer_onnx.cpp`
- **描述**: `ModelImp` 类中 `fe`、`m_session`、`vocab` 均为裸指针，构造函数中 `new`，析构函数中未正确释放（或根本没有析构逻辑）。
- **影响**: 每次创建/销毁 ModelImp 都泄漏内存。
- **建议修复**: 改用 `std::unique_ptr`，保证异常安全。
- **工时**: S（半天）

### H-02: audio-util 代码重复 [Lib]

- **位置**: `src/infer/audio-util/src/Util.cpp`
- **描述**: 存在与其他模块重复的工具函数实现。
- **影响**: 维护成本增加，改一处忘另一处时产生行为不一致。
- **建议修复**: 提取公共函数到 dstools-core 或专用工具模块。
- **工时**: S（半天）

### H-03: Mp3Decoder 线程安全问题 [Lib]

- **位置**: `src/infer/audio-util/src/Mp3Decoder.cpp`
- **描述**: Mp3Decoder 的解码状态非线程安全，多线程同时解码不同文件时会数据竞争。
- **影响**: 并发场景下解码结果错误或崩溃。
- **建议修复**: 为解码操作添加互斥保护，或确保每线程使用独立实例。
- **工时**: S（半天）

### H-04: Mp3Decoder 帧数计算错误 [Lib]

- **位置**: `src/infer/audio-util/src/Mp3Decoder.cpp`
- **描述**: 帧数计算逻辑有误，导致返回的总帧数不准确。
- **影响**: 进度条显示不准，seek 定位偏移。
- **建议修复**: 修正帧数计算公式。
- **工时**: S（半天）

### H-06: FunAsr 使用已废弃的 wstring_convert [Lib]

- **位置**: `src/infer/FunAsr/src/commonfunc.h`
- **描述**: 使用 `std::wstring_convert`，此 API 在 C++17 中已标记为 deprecated，将在未来标准中移除。
- **影响**: 编译器警告，未来标准升级后编译失败。
- **建议修复**: 改用平台 API 或第三方 UTF 转换库。
- **工时**: S（半天）

### H-07: GameModel 静态缓存 Bug [Lib]

- **位置**: `src/infer/game-infer/src/GameModel.cpp`
- **描述**: 模型推理结果使用 `static` 局部变量缓存，多次调用或多实例场景下返回过期数据。
- **影响**: 推理结果错误。
- **建议修复**: 将缓存改为实例成员变量。
- **工时**: S（半天）

### H-08: FFmpegDecoder goto 控制流 [Lib]

- **位置**: `src/audio/`（FFmpegDecoder 相关代码）
- **描述**: 使用 `goto` 做错误处理跳转，跳过了中间变量的构造/析构，导致资源泄漏和未定义行为。
- **影响**: 资源泄漏，可能的 UB。
- **建议修复**: 重构为 RAII + 提前 return 模式。
- **工时**: M（1~2天）

### H-09: FunAsr 全局无 RAII [Lib]

- **位置**: `src/infer/FunAsr/` 全模块
- **描述**: 整个 FunAsr 模块使用 C 风格手动内存管理，没有任何 RAII 包装。
- **影响**: 异常路径下资源泄漏，整体代码脆弱。
- **建议修复**: 逐步用 `std::unique_ptr` / `std::vector` 替换裸指针和手动 malloc/free。长远看建议评估替换整个 FunAsr 库。
- **工时**: L（2~3天）
- **关联**: CQ-008

### H-10: FFmpegDecoder initDecoder 资源泄漏 [Lib]

- **位置**: `src/audio/`（FFmpegDecoder 相关代码）
- **描述**: `initDecoder()` 在中途失败时未释放已分配的 FFmpeg 上下文（AVFormatContext、AVCodecContext 等）。
- **影响**: 打开损坏文件或不支持的格式时泄漏内存。
- **建议修复**: 在每个错误分支中释放已分配的资源，或使用 RAII 包装器。
- **工时**: M（1天）

---

## Medium (16)

### BUG-008: QDir::count() 包含 . 和 .. [App]

- **位置**: `src/apps/MinLabel/gui/MainWindow.cpp`
- **描述**: 用 `QDir::count()` 判断目录是否为空，但该方法返回值包含 `.` 和 `..` 两个特殊条目，空目录返回 2 而非 0。
- **影响**: 空目录被误判为"有文件"，逻辑错误。
- **建议修复**: 改用 `dir.entryList(filters, QDir::Files).count()`。
- **工时**: XS（5分钟）

### BUG-013: f0Values 空向量 UB [App]

- **位置**: PitchLabeler `PianoRollView.cpp`（需验证是否存在同类问题）
- **描述**: 对 `f0Values` 调用 `std::min_element` / `std::max_element` 时未检查是否为空，空向量上调用这些算法属于未定义行为。
- **影响**: 加载空 f0 数据的 .ds 文件时崩溃。
- **建议修复**: 在调用前检查 `isEmpty()`，为空时提示错误并 return。
- **工时**: XS（15分钟）
- **状态**: 需在 PitchLabeler 中验证是否存在同类问题。

### BUG-023: FunAsr ftell 返回值截断 [App]

- **位置**: `src/infer/FunAsr/src/util.cpp`
- **描述**: `uint32_t nFileLen = ftell(fp);`。`ftell` 返回 `long`，用 `uint32_t` 接收会丢失错误标志 `-1`，在 64 位系统上大文件的大小也会被截断。
- **影响**: 大模型文件加载时读取长度错误，静默数据损坏。
- **建议修复**: 用 `long` 接收，检查 `-1` 错误，再转 `size_t`。
- **工时**: XS（15分钟）

### BUG-024: splitPitch 越界访问 [App]

- **位置**: PitchLabeler `PianoRollView.cpp` 中的 `parseNoteName()`（需验证是否存在同类越界问题）
- **描述**: `notePitch.split(...)` 结果直接取 `[1]` 元素，当音符名不含偏移量（如 "C4" 无 "+5"）时 `splitPitch` 只有一个元素，`[1]` 越界。
- **影响**: 加载纯音符名时崩溃。
- **建议修复**: 检查 `splitPitch.size() > 1` 后再访问。
- **工时**: XS（5分钟）
- **状态**: 需在 PitchLabeler 中验证是否存在同类越界问题。

### BUG-025: FunAsr 类型双关 UB [App]

- **位置**: `src/infer/FunAsr/src/FeatureExtract.cpp`
- **描述**: `const float min_resol = *((float *) &val);` 通过强制指针转换做类型双关，违反 strict aliasing 规则，属于未定义行为。
- **影响**: 优化编译器可能生成错误代码。
- **建议修复**: 改用 `std::memcpy`。
- **工时**: XS（5分钟）

### H-12: FunAsr FeatureQueue 析构泄漏 [Lib]

- **位置**: `src/infer/FunAsr/src/FeatureQueue.cpp`
- **描述**: FeatureQueue 析构函数未清理队列中残留的动态分配对象。
- **影响**: 程序退出时泄漏。正常使用中如果识别被中途取消，也会泄漏。
- **建议修复**: 析构函数中遍历队列释放所有元素。
- **工时**: S（半天）

### H-13: FunAsr Audio 析构无清理 [Lib]

- **位置**: `src/infer/FunAsr/src/Audio.cpp`
- **描述**: Audio 类析构函数为空或缺失，内部持有的动态缓冲区不会被释放。
- **影响**: 每次 Audio 对象销毁时泄漏。
- **建议修复**: 在析构函数中释放所有动态分配的成员。
- **工时**: XS（30分钟）

### H-14: paraformer_onnx 悬空指针 [Lib]

- **位置**: `src/infer/FunAsr/src/paraformer_onnx.cpp`
- **描述**: 内部指针在某些错误路径下被释放后仍可能被引用。
- **影响**: Use-after-free。
- **建议修复**: 配合 BUG-022 一起改用 `std::unique_ptr`。
- **工时**: S（半天，与 BUG-022 合并处理）

### M-01: 命名空间注释写错 [Lib]

- **位置**: `src/infer/audio-util/src/SndfileVio.cpp`
- **描述**: 命名空间闭合大括号的注释标注了错误的命名空间名。
- **影响**: 可读性降低，维护时产生混淆。
- **建议修复**: 修正注释。
- **工时**: XS（5分钟）

### M-02: Include guard 名称不匹配 [Lib]

- **位置**: `src/infer/audio-util/src/MathUtils.h`
- **描述**: `#ifndef` 中的宏名与文件名不匹配。
- **影响**: 极端情况下可能导致重复包含。
- **建议修复**: 统一宏名与文件名。
- **工时**: XS（5分钟）

### M-03: FlacDecoder 硬编码中文错误信息 [Lib]

- **位置**: `src/infer/audio-util/src/FlacDecoder.cpp`
- **描述**: 错误信息用中文硬编码，未用 `tr()` 包裹。
- **影响**: 非中文用户无法理解错误信息，阻碍国际化。
- **建议修复**: 改为英文并用 `tr()` 包裹。
- **工时**: XS（15分钟）
- **关联**: CQ-003（i18n）

### M-05: Rmvpe 公有成员应改为私有 [Lib]

- **位置**: `src/infer/rmvpe-infer/include/rmvpe-infer/Rmvpe.h`
- **描述**: 内部实现细节作为 public 成员暴露。
- **影响**: 破坏封装性，外部代码可能误用。
- **建议修复**: 将实现细节移到 private 区域，必要时提供公共接口。
- **工时**: S（半天）

### M-06: 未使用的 Dur 结构体 [Lib]

- **位置**: `src/infer/some-infer/include/some-infer/Some.h`
- **描述**: 头文件中定义了 `Dur` 结构体，但代码中从未使用。
- **影响**: 增加代码理解成本。
- **建议修复**: 确认无引用后删除。
- **工时**: XS（5分钟）

### M-11: 错误输出到 stdout 而非 stderr [Lib]

- **位置**: `src/infer/audio-util/src/Util.cpp`
- **描述**: 错误日志通过 `printf` 或 `std::cout` 输出到 stdout，应输出到 stderr。
- **影响**: 管道操作时错误信息混入正常输出。
- **建议修复**: 改用 `fprintf(stderr, ...)` 或 `qWarning()`。
- **工时**: XS（5分钟）

### CQ-005: 循环内创建 QRegularExpression

- **位置**: `src/apps/MinLabel/gui/MainWindow.cpp`
- **描述**: 在循环体内反复构造 QRegularExpression 对象。正则编译是昂贵操作。
- **影响**: 性能损耗（文件量大时明显）。
- **建议修复**: 提取为 `static const QRegularExpression`。
- **工时**: XS（15分钟）

### CQ-006a: FaTread 命名错误未修正

- **位置**: `src/apps/pipeline/lyricfa/` 相关文件
- **描述**: `FaTread` 应为 `LyricMatchTask`，共 9 处引用，还有文件名需要重命名。
- **影响**: 代码可读性差。
- **建议修复**: 全局重命名，包括文件名。
- **工时**: S（半天）

---

## Low (15)

### L-02: AsyncTaskWindow UI 字符串未国际化 [Lib]

- **位置**: `src/widgets/`
- **描述**: 异步任务窗口中的 UI 字符串未用 `tr()` 包裹。
- **影响**: 无法翻译。
- **建议修复**: 用 `tr()` 包裹所有用户可见字符串。
- **工时**: S（半天）

### L-03: TaskWindow 文件过滤器仅支持 WAV [Lib]

- **位置**: `src/widgets/`
- **描述**: 文件选择对话框的过滤器只列出 WAV，实际代码已支持 MP3、FLAC 等格式。
- **影响**: 用户无法直接选取其他格式的文件。
- **建议修复**: 扩展过滤器列表。
- **工时**: XS（5分钟）

### L-04: 残留 debug print 语句 [Lib]

- **位置**: `src/infer/audio-util/src/Util.cpp`
- **描述**: 遗留的 debug 打印语句未清理。
- **影响**: 控制台输出噪音。
- **建议修复**: 删除或改为条件编译。
- **工时**: XS（5分钟）

### L-05: 死代码 tmp.h [Lib]

- **位置**: `src/infer/FunAsr/src/tmp.h`
- **描述**: 文件内容未被任何代码引用，属于死代码。
- **影响**: 增加代码库噪音。
- **建议修复**: 确认无引用后删除。
- **工时**: XS（5分钟）

### L-06: 宏定义写在命名空间内部 [Lib]

- **位置**: `src/infer/FunAsr/include/ComDefine.h`
- **描述**: `#define` 宏放在 `namespace` 块内，宏不受命名空间约束，放在里面只会产生误解。
- **影响**: 可读性和意图混淆。
- **建议修复**: 将宏移到命名空间外部，或改为 `constexpr`。
- **工时**: S（半天）

### L-07: 注释掉的代码块 [Lib]

- **位置**: `src/infer/rmvpe-infer/tests/main.cpp`
- **描述**: 大段被注释掉的测试代码未清理。
- **影响**: 降低可读性。
- **建议修复**: 删除（有 git 历史可查）。
- **工时**: XS（5分钟）

### L-08: IAudioEncoder 空壳接口 [Lib]

- **位置**: `src/audio/`（如仍存在）
- **描述**: IAudioEncoder 接口定义存在但无实现、无使用者。
- **影响**: 代码库噪音。
- **建议修复**: 确认无计划使用后删除。
- **工时**: XS（5分钟）

### L-14: HfaThread.cpp~ 备份文件残留

- **位置**: `src/apps/HubertFA/util/HfaThread.cpp~`
- **描述**: 编辑器备份文件残留在版本控制中。
- **影响**: 仓库噪音。
- **建议修复**: 删除文件，添加 `*~` 到 `.gitignore`。
- **工时**: XS（5分钟）

### L-13: Ort::MemoryInfo 重复创建 [Lib]

- **位置**: `src/infer/game-infer/src/GameModel.cpp`
- **描述**: 每次推理调用都重新创建 `Ort::MemoryInfo::CreateCpu`，该对象可复用。
- **影响**: 微小性能损耗。
- **建议修复**: 改为类成员变量，构造时创建一次。
- **工时**: XS（15分钟）

### CQ-003: 零国际化支持

- **位置**: 全项目
- **描述**: 整个项目没有使用 `tr()`，界面字符串混用中英文，全部硬编码。
- **影响**: 无法翻译到其他语言。
- **建议修复**: 逐文件添加 `tr()` 并创建翻译文件。
- **工时**: XL（3~5天）
- **关联**: L-02, M-03

### CQ-008: 裸 new/delete 遍布全项目

- **位置**: 全项目（FunAsr 最严重）
- **描述**: 大量手动内存管理，缺乏 RAII 包装。
- **影响**: 异常路径下资源泄漏。
- **建议修复**: 逐步替换为智能指针。
- **工时**: L（2~3天）
- **关联**: H-09, BUG-007, BUG-022

### CQ-009: 错误处理风格不一致

- **位置**: 全项目
- **描述**: 有的用返回码，有的用异常，有的直接忽略，风格混乱。
- **影响**: 调用者不知道如何处理错误。
- **建议修复**: 设计统一的 Result/Status 类型。
- **工时**: L（2~3天）
- **依赖**: AD-03（ErrorHandling 模块）

### CQ-010: FunAsr 魔法数字

- **位置**: `src/infer/FunAsr/`
- **描述**: 512、8404、1024、mel 滤波器系数等数值直接硬编码，无命名常量。
- **影响**: 可读性极差，修改时容易遗漏。
- **建议修复**: 提取为命名常量。
- **工时**: M（1天）

### BUG-016: Vocab str2int 边界检查缺失 [App]

- **位置**: `src/infer/FunAsr/src/Vocab.cpp`
- **描述**: `str2int` 函数在字符串长度 < 3 时直接访问 `ch_array[0..2]`，越界读取。
- **影响**: 短字符串输入时越界访问。
- **建议修复**: 添加 `if (str.size() < 3) return 0;` 前置检查。
- **工时**: XS（5分钟）

### BUG-012: GameInfer 错误提示不当 [App]

- **位置**: `src/apps/GameInfer/`
- **描述**: 错误提示语法错误（"is not exists"）、参数传递不当（toLocal8Bit 多余转换）、使用 information 级别弹出错误。
- **影响**: 用户体验差。
- **建议修复**: 修正文案、简化参数、改用 `QMessageBox::critical`。
- **工时**: XS（15分钟）

---

## 统计

| 严重度 | 数量 | 预估总工时 |
|--------|------|-----------|
| Critical | 5 | ~2天 |
| High | 14 | ~12天 |
| Medium | 16 | ~7天 |
| Low | 15 | ~10天 |
| **合计** | **50** | **~31天** |

### 模块分布

| 模块 | Bug 数量 | 占比 |
|------|---------|------|
| FunAsr | 16 | 32% |
| audio (SDL/FFmpeg) | 9 | 18% |
| PitchLabeler (待验证) | 2 | 4% |
| 全项目 | 5 | 10% |
| HubertFA | 3 | 6% |
| audio-util | 5 | 10% |
| MinLabel | 2 | 4% |
| 其他 | 8 | 16% |

### 去重说明

以下 Bug 在多个来源中重复出现，本文档以应用层 ID 为主，库级别 ID 作为别名：

| 主 ID | 别名 | 说明 |
|-------|------|------|
| BUG-018 | H-01, H-05 | volatile 误用 + 忙等待（同一问题） |
| BUG-019 | H-11 | SDL mutex 无 RAII |
| CQ-011 | C-05 | pcm_buffer 过度分配 |

### 工时估算说明

| 级别 | 时间 |
|------|------|
| XS | < 30分钟 |
| S | 半天 |
| M | 1~2天 |
| L | 2~3天 |
| XL | 3~5天 |
