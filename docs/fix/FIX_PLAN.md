# 修复方案

> 版本: 2.0 | 日期: 2026-04-26
> 共 30 个修复项, 覆盖 53 个问题

---

## Phase 1: 内存安全与线程安全 (CRITICAL + HIGH)

### Fix-01: FunAsr 文件 I/O 空指针检查
**对应**: C-01, C-02 | **文件**: `FunAsr/src/util.cpp`

```cpp
// loadparams - 在 fopen 后添加:
FILE *fp = fopen(filename, "rb");
if (!fp) {
    return nullptr;  // 或抛出异常
}

// SaveDataFile - 在 fopen 后添加:
FILE *fp = fopen(filename, "wb");
if (!fp) {
    return;  // 或返回错误码
}
```

### Fix-02: SpeechWrap 缓冲区边界检查 + 生命周期管理
**对应**: C-03, C-07 | **文件**: `FunAsr/src/SpeechWrap.cpp`, `FunAsr/src/SpeechWrap.h`

```cpp
// 1. cache 改为动态分配
std::vector<float> cache;  // 替换 float cache[400]

// update() 中:
int cache_size = total_size - offset;
cache.resize(cache_size);
memcpy(cache.data(), speech_data + offset, cache_size * sizeof(float));

// 2. in 指针生命周期管理 - 使用 shared_ptr 或存储副本
std::vector<float> owned_data;  // 拥有数据副本
void load(const float* data, int size) {
    owned_data.assign(data, data + size);
    // ...
}
float operator[](int index) {
    return owned_data[index];  // 安全访问
}
```

### Fix-03: FunAsr 所有 malloc/aligned_malloc 检查
**对应**: C-04, C-06, L-17 | **文件**: `FunAsr/src/Audio.cpp`, `FunAsr/src/util.cpp`

```cpp
// Audio.cpp:
speech_data = (float*)malloc(speech_len * sizeof(float));
if (!speech_data) {
    return;  // 或抛出 std::bad_alloc
}

// util.cpp loadparams:
char *params = (char *)aligned_malloc(32, nFileLen);
if (!params) {
    fclose(fp);
    return nullptr;
}

// util.cpp softmax / log_softmax:
float *tmpd = (float *)malloc(Tsz * sizeof(float));
if (!tmpd) return;  // 安全退出
```

### Fix-04: SDLPlayback 缓冲区大小修正
**对应**: C-05 | **文件**: `sdlplayback/private/SDLPlayback_p.cpp`

```cpp
// 修改前:
pcm_buffer = new float[pcm_buffer_size * sizeof(float)];
// 修改后:
pcm_buffer = new float[pcm_buffer_size];
```

### Fix-05: SDLPlayback 消除忙等 + lock_guard
**对应**: H-01, H-11 | **文件**: `sdlplayback/private/SDLPlayback_p.cpp`

```cpp
// 1. 引入 mutex + condition_variable
std::mutex state_mtx;
std::condition_variable state_cv;

// play() 中:
{
    std::unique_lock<std::mutex> lock(state_mtx);
    state_cv.wait(lock, [this]{ return state != Playing; });
}

// 状态变更处:
{
    std::lock_guard<std::mutex> lock(state_mtx);
    state = newState;
}
state_cv.notify_all();

// 2. workCallback 中:
// 修改前:
mutex.lock();
// ... 操作 ...
mutex.unlock();
// 修改后:
{
    std::lock_guard<std::mutex> guard(mutex);
    // ... 操作 ...
}
```

### Fix-06: IAudioPlayback volatile -> atomic
**对应**: H-05 | **文件**: `qsmedia/Api/private/IAudioPlayback_p.h`

```cpp
// 修改前:
volatile int state;
// 修改后:
#include <atomic>
std::atomic<int> state;
```

### Fix-07: mpg123 单次初始化
**对应**: H-03 | **文件**: `audio-util/src/Mp3Decoder.cpp`

```cpp
#include <mutex>

static void initMpg123() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        mpg123_init();
    });
}
// 在解码函数入口调用 initMpg123()
// 移除 mpg123_exit() 调用 (进程退出时自动清理)
```

### Fix-08: GameModel 静态变量保护 + 不缓存模型数据
**对应**: H-07 | **文件**: `game-infer/src/GameModel.cpp`

```cpp
// 问题: static 变量缓存了首次模型的 input names, 后续模型复用错误值
// 修改: 移除 static, 每次从当前 session 获取
std::vector<std::string> segInputNames;  // 非 static
// ... 初始化逻辑 ...
```

### Fix-09: FunAsr RAII 改造 (关键路径)
**对应**: H-09, H-12, H-13, L-09 | **涉及文件**: FunAsr 多个文件

**策略**: 分步进行, 优先改造最危险的路径:

1. **Tensor Rule of Five** (`Tensor.h`):
```cpp
// 禁止拷贝, 允许移动
Tensor(const Tensor&) = delete;
Tensor& operator=(const Tensor&) = delete;
Tensor(Tensor&& other) noexcept : buff(other.buff), size(other.size) {
    other.buff = nullptr;
    other.size = 0;
}
Tensor& operator=(Tensor&& other) noexcept {
    if (this != &other) {
        aligned_free(buff);
        buff = other.buff; size = other.size;
        other.buff = nullptr; other.size = 0;
    }
    return *this;
}
```

2. **FeatureQueue 析构** (`FeatureQueue.cpp`):
```cpp
FeatureQueue::~FeatureQueue() {
    while (!feature_queue.empty()) {
        delete feature_queue.front();
        feature_queue.pop();
    }
    delete buff;
}
```

3. **Audio 析构** (`Audio.cpp`):
```cpp
Audio::~Audio() {
    while (!frame_queue.empty()) {
        delete frame_queue.front();
        frame_queue.pop();
    }
    if (speech_data) free(speech_data);
    if (speech_buff) free(speech_buff);
}
```

4. `FeatureQueue` 中 `Tensor*` -> `std::unique_ptr<Tensor>`
5. `util.cpp` 中 `loadparams` 返回 `std::unique_ptr<char[]>`
6. `Audio.cpp` 中 `speech_data` -> `std::vector<float>`

### Fix-10: FFmpegDecoder initDecoder 资源泄漏修复
**对应**: H-10 | **文件**: `ffmpegdecoder/private/FFmpegDecoder_p.cpp`

```cpp
bool FFmpegDecoderPrivate::initDecoder(const QString &filename) {
    // ... 分配 _formatContext ...

    if (avformat_find_stream_info(_formatContext, nullptr) < 0) {
        quitDecoder();  // 添加清理
        return false;
    }
    // 对每个后续错误检查点都添加 quitDecoder():
    // lines 58, 69, 80, 90, 121, 136, 147, 157
    // ...
}
```

### Fix-11: paraformer_onnx 悬空指针修复
**对应**: H-14 | **文件**: `FunAsr/src/paraformer_onnx.cpp`

```cpp
// 修改: 先收集所有 string, 最后再构建 const char* 数组
// 1. 收集阶段 (循环中):
m_strInputNames.emplace_back(strName);
// 不在此处构建 m_szInputNames

// 2. 收集完毕后:
for (const auto& name : m_strInputNames) {
    m_szInputNames.push_back(name.c_str());
}
// 此时 m_strInputNames 不会再重分配, c_str() 稳定
```

### Fix-12: FFmpegDecoder decode NULL 缓冲区保护
**对应**: M-22 | **文件**: `ffmpegdecoder/private/FFmpegDecoder_p.cpp`

```cpp
// 在 memcpy 前添加:
if (buf != nullptr) {
    memcpy(buf, _cache.data() + _cacheRead, cacheTransferSize);
}
// skip 操作时 buf 为 NULL, 仅更新偏移量
_cacheRead += cacheTransferSize;
```

### Fix-13: SDLPlayback 空设备列表检查
**对应**: L-18 | **文件**: `sdlplayback/private/SDLPlayback_p.cpp`

```cpp
auto devs = devices();
if (devs.empty()) {
    qWarning() << "No audio output devices available";
    return false;
}
auto deviceName = devs.front();
```

---

## Phase 2: 逻辑正确性与代码重复消除

### Fix-14: Mp3Decoder 帧数修正
**对应**: H-04 | **文件**: `audio-util/src/Mp3Decoder.cpp`

```cpp
// 修改前:
outBuf.writef(pcm_buffer.data(), num_samples);
// 修改后:
outBuf.writef(pcm_buffer.data(), num_samples / channels);
```

### Fix-15: SndfileVio 全面边界检查
**对应**: M-15, M-16, M-17, M-18 | **文件**: `audio-util/src/SndfileVio.cpp`, `audio-util/include/audio-util/SndfileVio.h`

```cpp
// 1. SndfileVio.h - info 零初始化:
struct SF_VIO {
    SF_INFO info{};  // 值初始化为零
    // ...
};

// 2. qvio_seek - 边界 clamp:
static sf_count_t qvio_seek(sf_count_t offset, int whence, void *user_data) {
    auto *qvio = static_cast<SF_VIO *>(user_data);
    sf_count_t newPos;
    switch (whence) {
        case SEEK_SET: newPos = offset; break;
        case SEEK_CUR: newPos = qvio->seek + offset; break;
        case SEEK_END: newPos = qvio->byteArray.size() + offset; break;
        default: return -1;
    }
    // clamp
    if (newPos < 0) newPos = 0;
    if (newPos > (sf_count_t)qvio->byteArray.size())
        newPos = qvio->byteArray.size();
    qvio->seek = newPos;
    return newPos;
}

// 3. qvio_read - 越界保护:
static sf_count_t qvio_read(void *ptr, sf_count_t count, void *user_data) {
    auto *qvio = static_cast<SF_VIO *>(user_data);
    if (qvio->seek >= (uint64_t)qvio->byteArray.size())
        return 0;  // 已到末尾
    sf_count_t remaining = qvio->byteArray.size() - qvio->seek;
    sf_count_t bytesToRead = std::min(count, remaining);
    memcpy(ptr, qvio->byteArray.data() + qvio->seek, bytesToRead);
    qvio->seek += bytesToRead;
    return bytesToRead;
}

// 4. qvio_write - 更新 seek:
static sf_count_t qvio_write(const void *ptr, sf_count_t count, void *user_data) {
    auto *qvio = static_cast<SF_VIO *>(user_data);
    qvio->byteArray.append(static_cast<const char *>(ptr), count);
    qvio->seek = qvio->byteArray.size();  // 更新 seek
    return count;
}
```

### Fix-16: FunAsr Vocab 安全修复
**对应**: M-12, M-19 | **文件**: `FunAsr/src/Vocab.cpp`

```cpp
// 1. 安全大写转换:
// 修改前:
word[0] = word[0] - 32;
// 修改后:
if (!word.empty()) {
    word[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(word[0])));
}

// 2. vector2stringV2 越界检查:
for (auto it = results.begin(); it != results.end(); ++it) {
    if (*it < 0 || *it >= (int)vocab.size()) continue;  // 跳过无效 ID
    std::string word = vocab[*it];
    // ...
}
```

### Fix-17: FunAsr log_softmax 数值稳定性
**对应**: M-20 | **文件**: `FunAsr/src/util.cpp`

```cpp
void log_softmax(float *din, int mask, int Tsz) {
    // 先找最大值
    float maxVal = din[0];
    for (int i = 1; i < Tsz; i++) {
        if (din[i] > maxVal) maxVal = din[i];
    }
    // 减去最大值后计算
    float sum = 0.0f;
    for (int i = 0; i < Tsz; i++) {
        sum += expf(din[i] - maxVal);
    }
    float logSum = logf(sum) + maxVal;
    for (int i = 0; i < Tsz; i++) {
        din[i] = din[i] - logSum;
    }
}
```

### Fix-18: 创建 infer-common 共享库
**对应**: M-04, L-01 | **新建**: `src/libs/infer-common/`

目录结构:
```
infer-common/
├── CMakeLists.txt
├── include/infer-common/
│   ├── InferCommonGlobal.h
│   ├── Provider.h          # 统一 ExecutionProvider 枚举
│   ├── OnnxUtils.h         # initDirectML, initCUDA, Ort::MemoryInfo 缓存
│   └── MusicUtils.h        # cumulativeSum, calculateNoteTicks, calculateSumOfDifferences
└── src/
    ├── OnnxUtils.cpp
    └── MusicUtils.cpp
```

**迁移步骤**:
1. 创建 infer-common 库并提取公共代码
2. some-infer、rmvpe-infer、game-infer 添加对 infer-common 的依赖
3. 删除各模块中的重复实现
4. 编译验证

### Fix-19: audio-util 声道转换去重
**对应**: H-02 | **文件**: `audio-util/src/Util.cpp`

```cpp
static void convertChannels(const float* input, float* output,
                            int frames, int srcChannels, int dstChannels) {
    // 声道转换逻辑 (仅一份)
}
// 主循环和 flush 循环都调用此函数
```

### Fix-20: WaveStream 采样数修正
**对应**: M-09 | **文件**: `qsmedia/NAudio/WaveStream.cpp`

```cpp
qint64 WaveStream::TotalSamples() const {
    return Length() / BlockAlign();  // BlockAlign = channels * bytesPerSample
}
```

### Fix-21: FeatureExtract 消除 UB
**对应**: M-13 | **文件**: `FunAsr/src/FeatureExtract.cpp`

```cpp
// 修改前:
int val = 0x34000000;
float min_resol = *((float*)&val);
// 修改后:
int val = 0x34000000;
float min_resol;
std::memcpy(&min_resol, &val, sizeof(float));
```

### Fix-22: game-infer bool 指针修复
**对应**: M-23 | **文件**: `game-infer/src/GameModel.cpp`

```cpp
// 修改前:
reinterpret_cast<bool*>(tempMaskT.data())
// 修改后 - 直接使用 uint8_t 指针, 或逐元素转换:
const uint8_t* maskData = tempMaskT.data();
// 在需要 bool 的地方: maskData[i] != 0
```

### Fix-23: FFmpegDecoder extractInt 类型安全
**对应**: M-21 | **文件**: `ffmpegdecoder/FFmpegDecoder.cpp`

```cpp
// 修改前:
int res = 0;
res = var.toLongLong();  // 截断
// 修改后:
qint64 extractInt64(const QVariant &var) {
    // 返回 qint64 避免截断
}
// 或在赋值处检查范围:
auto val = var.toLongLong();
if (val > INT_MAX || val < INT_MIN) {
    qWarning() << "Value truncated";
}
res = static_cast<int>(val);
```

### Fix-24: cumulativeSum 空输入保护
**对应**: L-11 | **文件**: `some-infer/src/Some.cpp`, `game-infer/src/Game.cpp`

```cpp
static std::vector<int64_t> cumulativeSum(const std::vector<int64_t> &durations) {
    if (durations.empty()) return {};
    std::vector<int64_t> cumsum(durations.size());
    cumsum[0] = durations[0];
    // ...
}
```

---

## Phase 3: 构建兼容与代码清理

### Fix-25: FunAsr CMakeLists 修复
**对应**: M-07, M-08 | **文件**: `FunAsr/src/CMakeLists.txt`

```cmake
# 修改前:
target_include_directories(${PROJECT_NAME} PUBLIC *.h)
# 修改后:
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 修改前:
target_include_directories(${PROJECT_NAME} PUBLIC ../../onnxruntime/include)
# 修改后:
target_include_directories(${PROJECT_NAME} PUBLIC ${ONNXRUNTIME_INCLUDE_DIR})
```

### Fix-26: 替换弃用 API
**对应**: H-06, M-10

- `FunAsr/commonfunc.h`: 替换 `std::wstring_convert` 为 Windows `MultiByteToWideChar`/`WideCharToMultiByte`
- `ffmpegdecoder/FFmpegDecoder.cpp`: `var.type()` -> `var.typeId()` 或 `var.metaType()`

### Fix-27: game-infer 跨平台 + 构建修复
**对应**: M-14, M-24, L-12 | **文件**: `game-infer/CMakeLists.txt`, `game-infer/src/GameModel.cpp`

```cmake
# CMakeLists.txt:
target_link_libraries(${PROJECT_NAME} PRIVATE onnxruntime)  # 改 PUBLIC 为 PRIVATE
```

```cpp
// GameModel.cpp - 跨平台文件路径:
#ifdef _WIN32
    std::ifstream configFile(path.wstring());
#else
    std::ifstream configFile(path.string());
#endif
```

### Fix-28: FunAsr Vocab CRLF + aligned_malloc 修复
**对应**: L-14, L-10 | **文件**: `FunAsr/src/Vocab.cpp`, `FunAsr/src/alignedmem.cpp`

```cpp
// Vocab.cpp wchar_t 构造函数 - 添加 CRLF 处理:
while (std::getline(file, line)) {
    if (!line.empty() && line.back() == L'\r')
        line.pop_back();
    // ...
}

// alignedmem.cpp - 验证对齐参数:
void* aligned_malloc(size_t alignment, size_t size) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return nullptr;  // 非 2 的幂
    }
    // ...
}
```

### Fix-29: 代码清理批量操作

| 操作 | 文件 | 对应 |
|------|------|------|
| 修复命名空间注释 | `audio-util/src/SndfileVio.cpp:53` | M-01 |
| 修复 include guard | `audio-util/src/MathUtils.h:18` | M-02 |
| 中文消息改英文 | `audio-util/src/FlacDecoder.cpp:28` | M-03 |
| 删除 tmp.h | `FunAsr/src/tmp.h` | L-05 |
| 删除注释代码 | `rmvpe-infer/tests/main.cpp:70-85` | L-07 |
| 删除未使用 Dur | `some-infer/include/some-infer/Some.h:19-21` | M-06 |
| 宏改 constexpr | `FunAsr/include/ComDefine.h:5-9` | L-06 |
| cout -> cerr | `audio-util/src/Util.cpp` 多处 | M-11 |
| 移除调试打印 | `audio-util/src/Util.cpp:449` 等 | L-04 |
| 文件过滤器扩展 | `AsyncTaskWindow.cpp:58` | L-03 |
| Rmvpe 成员改 private | `rmvpe-infer/include/rmvpe-infer/Rmvpe.h:33` | M-05 |
| divIntRound 除零保护 | `audio-util/src/MathUtils.h:8-16` | L-16 |
| #if -> #ifdef 统一 | `some-infer/SomeModel.cpp`, `rmvpe-infer/RmvpeModel.cpp` | L-12 |
| Slicer 有符号修复 | `audio-util/src/Slicer.cpp:126` | L-15 |
| MemoryInfo 缓存 | `game-infer/src/GameModel.cpp` 多处 | L-13 |

### Fix-30: FFmpegDecoder 重构 goto
**对应**: H-08 | **文件**: `ffmpegdecoder/private/FFmpegDecoder_p.cpp`

**策略**: 使用 RAII wrapper 封装 FFmpeg 资源 (AVFrame, AVPacket 等), 用 `do { ... } while(0)` + break 或函数提取替换 goto。此为较大重构, 建议单独 PR。

---

## 风险评估

| 修复 | 风险 | 缓解措施 |
|------|------|----------|
| Fix-09 (FunAsr RAII) | **高** - 大范围改动, 涉及 Tensor/Queue/Audio 生命周期 | 分步进行, 每步编译验证; 先禁拷贝再改智能指针 |
| Fix-11 (悬空指针) | **高** - 改变初始化顺序可能影响现有行为 | 添加单元测试验证模型加载 |
| Fix-18 (infer-common) | **中** - 影响 3 个模块的构建依赖 | 先提取, 不改接口, 逐模块迁移 |
| Fix-05 (条件变量) | **中** - 并发逻辑 | 充分测试播放/停止/切换场景 |
| Fix-15 (SndfileVio) | **中** - 改变 VIO 行为可能影响音频读写 | 用已有 WAV/FLAC 文件回归测试 |
| Fix-17 (log_softmax) | **中** - 改变数值结果 | 对比修改前后 ASR 输出结果 |
| Fix-30 (goto 重构) | **中** - 复杂控制流 | 对比重构前后解码结果 |
| 其余修复 | **低** - 局部修改 | 编译 + 基本功能测试 |

---

## 修复项与问题对照表

| 修复 | 覆盖问题 |
|------|----------|
| Fix-01 | C-01, C-02 |
| Fix-02 | C-03, C-07 |
| Fix-03 | C-04, C-06, L-17 |
| Fix-04 | C-05 |
| Fix-05 | H-01, H-11 |
| Fix-06 | H-05 |
| Fix-07 | H-03 |
| Fix-08 | H-07 |
| Fix-09 | H-09, H-12, H-13, L-09 |
| Fix-10 | H-10 |
| Fix-11 | H-14 |
| Fix-12 | M-22 |
| Fix-13 | L-18 |
| Fix-14 | H-04 |
| Fix-15 | M-15, M-16, M-17, M-18 |
| Fix-16 | M-12, M-19 |
| Fix-17 | M-20 |
| Fix-18 | M-04, L-01, L-13 |
| Fix-19 | H-02 |
| Fix-20 | M-09 |
| Fix-21 | M-13 |
| Fix-22 | M-23 |
| Fix-23 | M-21 |
| Fix-24 | L-11 |
| Fix-25 | M-07, M-08 |
| Fix-26 | H-06, M-10 |
| Fix-27 | M-14, M-24, L-12 |
| Fix-28 | L-14, L-10 |
| Fix-29 | M-01, M-02, M-03, M-05, M-06, M-11, L-03, L-04, L-05, L-06, L-07, L-15, L-16 |
| Fix-30 | H-08 |
