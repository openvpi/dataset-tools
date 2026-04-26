# 产品需求文档 (PRD): src/lib 代码质量改进

> 版本: 2.0 | 日期: 2026-04-26

---

## 1. 背景

`src/libs/` 包含 9 个 C++/Qt 模块, 构成 dataset-tools 的核心基础设施:
- **音频处理层**: audio-util (底层 I/O)、qsmedia (Qt 抽象)、ffmpegdecoder、sdlplayback
- **AI 推理层**: some-infer (旋律)、rmvpe-infer (F0)、game-infer (音符)、FunAsr (语音识别)
- **UI 基础层**: AsyncTaskWindow

经两轮深度代码审计, 共发现 **7 个严重问题、14 个高优先级问题、20 个中等问题、12 个低优先级问题 (共 53 个)**, 涵盖内存安全、线程安全、逻辑错误、资源泄漏、代码重复、API 设计等方面。

### 1.1 问题分布

| 模块 | CRITICAL | HIGH | MEDIUM | LOW | 合计 |
|------|----------|------|--------|-----|------|
| FunAsr | 4 | 5 | 5 | 5 | **19** |
| sdlplayback | 1 | 2 | 0 | 1 | 4 |
| ffmpegdecoder | 0 | 2 | 3 | 0 | 5 |
| audio-util | 0 | 2 | 5 | 3 | 10 |
| game-infer | 0 | 1 | 3 | 3 | 7 |
| qsmedia | 0 | 1 | 1 | 1 | 3 |
| some-infer | 0 | 0 | 1 | 1 | 2 |
| rmvpe-infer | 0 | 0 | 1 | 1 | 2 |
| 跨模块 | 0 | 0 | 1 | 1 | 2 |

**结论**: FunAsr 是问题最集中的模块 (19/53 = 36%), 应作为重点治理对象。

## 2. 目标

| 目标 | 衡量标准 |
|------|----------|
| 消除所有内存安全漏洞 | 0 个 CRITICAL 级别问题 |
| 修复线程安全问题 | 无 volatile 误用, 无忙等, 无数据竞争 |
| 消除资源泄漏 | FFmpeg 上下文、队列元素、文件句柄均正确释放 |
| 修复逻辑错误 | 帧数计算、边界检查、数值稳定性等全部修正 |
| 减少代码重复 | 推理模块共享代码提取为公共库, 重复行数减少 >80% |
| 统一错误处理 | 所有模块采用一致的错误传播策略 |
| 现代化 FunAsr | RAII 替换手动内存管理, 消除 UB, 修复 Rule of Five |
| 改善跨平台兼容性 | 修复 wstring、CRLF、平台条件编译等问题 |

## 3. 非目标

- 不改变模块的外部 API 签名 (保持 ABI 兼容)
- 不重写 qsmedia NAudio 架构
- 不合并 audio-util 和 qsmedia (架构变更过大)
- 不添加新功能

## 4. 需求详情

### 4.1 内存安全 (P0)

| 需求 ID | 描述 | 对应问题 |
|---------|------|----------|
| REQ-01 | FunAsr 所有文件 I/O 操作必须检查 fopen 返回值 | C-01, C-02 |
| REQ-02 | SpeechWrap::cache 必须动态分配或添加边界检查 | C-03 |
| REQ-03 | FunAsr 所有 malloc/aligned_malloc 调用必须检查返回值 | C-04, C-06, L-17 |
| REQ-04 | SDLPlayback 缓冲区分配修正为正确大小 | C-05 |
| REQ-05 | FunAsr 核心数据结构使用 unique_ptr/vector 替换裸指针 | H-09 |
| REQ-28 | SpeechWrap 生命周期管理, 防止悬空指针 | C-07 |
| REQ-29 | FFmpegDecoder initDecoder 所有错误路径释放资源 | H-10 |
| REQ-30 | FeatureQueue 析构函数排空队列并释放 Tensor | H-12 |
| REQ-31 | Audio 析构函数排空 frame_queue 并释放 AudioFrame | H-13 |
| REQ-32 | paraformer_onnx 修复 m_szInputNames 悬空指针 | H-14 |
| REQ-33 | FFmpegDecoder decode 在 buf==NULL 时跳过 memcpy | M-22 |
| REQ-34 | FunAsr Tensor 实现 Rule of Five 或禁止拷贝 | L-09 |

### 4.2 线程安全 (P0)

| 需求 ID | 描述 | 对应问题 |
|---------|------|----------|
| REQ-06 | SDLPlayback 状态等待改用条件变量 | H-01 |
| REQ-07 | IAudioPlayback state 改用 std::atomic | H-05 |
| REQ-08 | mpg123_init 使用 std::call_once 保证单次调用 | H-03 |
| REQ-09 | GameModel 静态变量初始化添加同步保护, 不缓存模型相关数据 | H-07 |
| REQ-35 | SDLPlayback workCallback 使用 lock_guard 替代手动 lock/unlock | H-11 |

### 4.3 逻辑正确性 (P1)

| 需求 ID | 描述 | 对应问题 |
|---------|------|----------|
| REQ-14 | Mp3Decoder writef 帧数除以声道数 | H-04 |
| REQ-15 | WaveStream 采样数计算考虑声道数 | M-09 |
| REQ-16 | FeatureExtract 类型双关改用 memcpy | M-13 |
| REQ-17 | Vocab 字符转换改用 std::toupper | M-12 |
| REQ-36 | SndfileVio qvio_read 添加 seek 越界检查 | M-15 |
| REQ-37 | SndfileVio qvio_seek 添加边界 clamp | M-16 |
| REQ-38 | SndfileVio qvio_write 更新 seek 位置 | M-17 |
| REQ-39 | SF_VIO::info 零初始化 | M-18 |
| REQ-40 | Vocab::vector2stringV2 添加 vocab 边界检查 | M-19 |
| REQ-41 | log_softmax 减去最大值防止溢出 | M-20 |
| REQ-42 | FFmpegDecoder extractInt 使用正确类型避免截断 | M-21 |
| REQ-43 | game-infer 用 uint8_t 替代 reinterpret_cast<bool*> | M-23 |
| REQ-44 | cumulativeSum/calculateNoteTicks 空输入保护 | L-11 |
| REQ-45 | MathUtils divIntRound 除零保护 | L-16 |
| REQ-46 | SDLPlayback devices 空列表检查 | L-18 |

### 4.4 代码重复消除 (P1)

| 需求 ID | 描述 | 对应问题 |
|---------|------|----------|
| REQ-10 | 创建 `infer-common` 共享库, 提取 initDirectML/initCUDA | M-04 |
| REQ-11 | 提取 cumulativeSum/calculateNoteTicks 到 infer-common | M-04 |
| REQ-12 | 统一 ExecutionProvider 枚举到 infer-common | L-01 |
| REQ-13 | audio-util 中重复的声道转换逻辑提取为函数 | H-02 |

### 4.5 构建与兼容性 (P2)

| 需求 ID | 描述 | 对应问题 |
|---------|------|----------|
| REQ-18 | FunAsr CMakeLists 修复无效 include 路径 | M-07 |
| REQ-19 | FunAsr CMakeLists 使用 CMake 变量替换硬编码路径 | M-08 |
| REQ-20 | FFmpegDecoder 替换弃用 Qt API | M-10 |
| REQ-21 | FunAsr 替换弃用 std::wstring_convert | H-06 |
| REQ-22 | game-infer onnxruntime 链接改为 PRIVATE | M-14 |
| REQ-47 | game-infer wstring 跨平台条件处理 | M-24 |
| REQ-48 | 统一 #if vs #ifdef CUDA 宏用法 | L-12 |
| REQ-49 | FunAsr Vocab wchar_t 构造函数处理 CRLF | L-14 |
| REQ-50 | aligned_malloc 验证 alignment 为 2 的幂 | L-10 |

### 4.6 代码清理 (P2)

| 需求 ID | 描述 | 对应问题 |
|---------|------|----------|
| REQ-23 | 移除调试打印, 统一使用日志框架或 qDebug | L-04, M-11 |
| REQ-24 | 移除死代码 (tmp.h, 注释代码块, 未使用结构体) | L-05, L-07, M-06 |
| REQ-25 | 修复注释/include guard 不匹配 | M-01, M-02 |
| REQ-26 | FunAsr 宏改为 constexpr/enum | L-06 |
| REQ-27 | AsyncTaskWindow 文件过滤器支持 mp3/flac | L-03 |

## 5. 优先级排期

| 阶段 | 内容 | 需求 | 预估工时 |
|------|------|------|----------|
| Phase 1 | P0: 内存安全 + 线程安全 | REQ-01~09, 28~35 | 5-8 天 |
| Phase 2 | P1: 逻辑正确性 + 代码重复 | REQ-10~17, 36~46 | 7-10 天 |
| Phase 3 | P2: 构建兼容 + 清理 | REQ-18~27, 47~50 | 4-5 天 |

**总预估**: 16-23 个工作日

## 6. 验收标准

1. 所有 CRITICAL/HIGH 问题修复并通过代码审查
2. 所有 MEDIUM 问题修复或有明确的技术原因豁免
3. 项目能在 MSVC/MinGW 上正常编译
4. 已有测试用例全部通过
5. 无新增编译器警告 (W4/Wall 级别)
6. Valgrind/ASan 无内存错误 (Linux CI)
7. 多线程场景下无 TSan 报告的数据竞争
8. 空输入 / 无设备 / 文件不存在等边界场景不崩溃
