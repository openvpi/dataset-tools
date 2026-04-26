# 03 — Bug 修复实施细节

**Version**: 3.1  
**Date**: 2026-04-26  
**前置**: 01-architecture.md, 02-module-spec.md  
**来源**: docs/ISSUES.md v3.0  
**变更**: v3.1 标记已在原位置直接修复的无风险 Bug（不依赖重构）。

本文档为每个 Bug 提供精确的修复位置（在重构后的新路径中）、修复代码和验证方法。

> **路径说明**: AudioSlicer/LyricFA/HubertFA 的代码现位于 `src/apps/pipeline/slicer|lyricfa|hubertfa/`。MinLabel/SlurCutter/GameInfer 路径不变。

> **修复状态标记**: ✅ 已在当前代码修复 | ⏳ 待重构时修复 | ⚠️ 需设计评审

---

## 修复策略

重构过程中修复所有 43 个 Issue，策略分三类：

1. **随合并修复** — 代码合并到新模块时自然修复（如 PlayWidget 合并时修 BUG-001）
2. **定点修复** — 在原文件的新位置直接修改
3. **架构消除** — 重构后问题自然消失（如删除插件体系消除了工厂相关复杂度）

---

## Critical (3)

### BUG-001: exit(-1) → 优雅降级 ⚠️

**新位置**: `src/widgets/src/PlayWidget.cpp`, `src/apps/MinLabel/MainWindow.cpp`

**PlayWidget 修复**（5 处 exit 中的 2 处）：

```cpp
// src/widgets/src/PlayWidget.cpp

PlayWidget::PlayWidget(QWidget *parent) : QWidget(parent) {
    // ... UI 布局 ...
    initAudio();
    if (m_valid) {
        connect(m_playback, &AudioPlayback::stateChanged,
                this, &PlayWidget::onPlaybackStateChanged);
        connect(m_playback, &AudioPlayback::deviceChanged,
                this, &PlayWidget::onDeviceChanged);
        connect(m_playback, &AudioPlayback::deviceAdded,
                this, &PlayWidget::onDeviceAdded);
        connect(m_playback, &AudioPlayback::deviceRemoved,
                this, &PlayWidget::onDeviceRemoved);
        reloadDevices();
    } else {
        m_playBtn->setEnabled(false);
        m_stopBtn->setEnabled(false);
        m_devBtn->setEnabled(false);
        m_slider->setEnabled(false);
    }
}

void PlayWidget::initAudio() {
    m_decoder = new dstools::audio::AudioDecoder();
    m_playback = new dstools::audio::AudioPlayback(this);

    if (!m_playback->setup(44100, 2, 1024)) {
        QMessageBox::warning(this, qApp->applicationName(),
            tr("Failed to initialize audio playback. "
               "Audio features will be disabled."));
        delete m_playback; m_playback = nullptr;
        delete m_decoder;  m_decoder = nullptr;
        m_valid = false;
        return;
    }
    m_playback->setDecoder(m_decoder);
    m_valid = true;
}

PlayWidget::~PlayWidget() {
    if (m_playback) {
        m_playback->stop();
        m_playback->dispose();
    }
    delete m_playback;
    delete m_decoder;
}

// 所有公共方法添加守卫
void PlayWidget::openFile(const QString &path) {
    if (!m_valid) return;
    // ... 原逻辑 ...
}

void PlayWidget::setPlaying(bool playing) {
    if (!m_valid) return;
    // ... 原逻辑 ...
}
```

**MinLabel MainWindow 修复**（5 处 exit 中的 3 处：saveFile 2 处 + labToJson 1 处）：

```cpp
// src/apps/MinLabel/MainWindow.cpp

// void saveFile → bool saveFile（2 处 exit → return false）
bool MainWindow::saveFile(const QString &filename) {
    // ... 构造 writeData ...

    if (!writeJsonFile(jsonFilePath, writeData)) {
        QMessageBox::critical(this, qApp->applicationName(),
            tr("Failed to write to file %1").arg(jsonFilePath));
        return false;   // 原: exit(-1)
    }

    QFile labFile(labFilePath);
    if (!labFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, qApp->applicationName(),
            tr("Failed to write to file %1").arg(labFilePath));
        return false;   // 原: exit(-1)
    }
    // ... 写入 lab 内容 ...
    return true;
}

// 调用者处理返回值
void MainWindow::onTreeCurrentChanged(...) {
    if (!m_currentFile.isEmpty()) {
        saveFile(m_currentFile);  // 失败不阻止导航
    }
    openFile(newFile);
}

// labToJson 中的 exit(-1) 同样修复为错误提示
void MainWindow::labToJson(const QString &dir) {
    // ... 批量转换 ...
    if (!writeJsonFile(jsonFilePath, writeData)) {
        QMessageBox::critical(this, qApp->applicationName(),
            tr("Failed to write to file %1").arg(jsonFilePath));
        continue;   // 原: exit(-1)，改为跳过当前文件继续处理
    }
}
```

**验证**: 启动时拔掉音频设备 → 应显示警告并禁用播放按钮，不崩溃。

---

### BUG-006: CSV 解析索引 ✅

**已修复**: `src/apps/AudioSlicer/slicer/workthread.cpp:261-265`  
capturedView 索引 0/1/2 → 1/2/3（index 0 是完整匹配，捕获组从 1 开始）。

**新位置**: `src/apps/pipeline/slicer/WorkThread.cpp`

```cpp
// 原代码 (line 261-265):
// qint64 mm = capturedView(0).toLongLong();  ← 错！
// qint64 ss = capturedView(1).toLongLong();
// qint64 zzz = capturedView(2).toLongLong();

// 修复:
qint64 mm = match.capturedView(1).toLongLong();
qint64 ss = match.capturedView(2).toLongLong();
qint64 zzz = match.capturedView(3).toLongLong();
```

**验证**: 导出 CSV 标记后重新导入，时间戳应精确匹配。

---

### BUG-015: FunAsr fopen NULL ✅

**已修复**: `src/libs/FunAsr/src/util.cpp` — loadparams 和 SaveDataFile 均添加了 fopen 返回值 NULL 检查。注意 BUG-023（ftell 返回值截断）尚未修复。

**新位置**: `src/infer/funasr/src/util.cpp`

```cpp
// loadparams (原函数名为 loadparams，非 LoadDataFile)
fp = fopen(filename, "rb");
if (!fp) {
    LOG("ERROR: Failed to open file: %s\n", filename);
    return nullptr;
}

// SaveDataFile
fp = fopen(filename, "wb+");
if (!fp) {
    LOG("ERROR: Failed to create file: %s\n", filename);
    return;
}
```

**验证**: 指定不存在的模型路径 → 应显示错误消息，不崩溃。

---

## High (7)

### BUG-009: 工作线程 QMessageBox ✅

**已修复**: `src/apps/LyricFA/util/AsrThread.cpp:24` — QMessageBox::critical 替换为 emit oneFailed 信号，由主线程处理错误提示。

**新位置**: `src/apps/pipeline/lyricfa/AsrTask.cpp`

```cpp
// 原代码 (AsrThread.cpp:24):
// QMessageBox::critical(nullptr, ...);  ← 线程不安全！(用于文件写入失败)

// 修复:
void AsrTask::run() {
    std::string msg;
    if (!m_asr->recognize(wavPath, msg)) {
        emit oneFailed(m_filename, QString::fromStdString(msg));
        return;   // 不弹对话框，通过信号传递错误
    }
    // ... 后续处理 ...
}
```

### BUG-016: Vocab str2int 边界 ⏳

**未修复**: `src/libs/FunAsr/src/Vocab.cpp:43` — 缺少 `str.size() < 3` 前置检查，短字符串将越界访问 `ch_array[0..2]`。注意现有错误路径返回 `0`（非 `-1`），修复时应保持一致。

**新位置**: `src/infer/funasr/src/Vocab.cpp`

```cpp
int str2int(const std::string &str) {  // 自由函数，非 Vocab 成员
    if (str.size() < 3) return 0;  // ← 新增（与现有错误路径返回值一致）
    const char *ch_array = str.c_str();
    // ... 原逻辑 ...
}
```

### BUG-017: WaveStream 虚析构 ⏳

**新位置**: `src/audio/src/WaveFormat.cpp`（WaveFormat 替代了原 WaveStream）

重构后 `WaveStream` 基类被移除（AudioDecoder 不再继承），此问题自然消除。如果保留 WaveFormat 继承体系，确保析构函数为 virtual。

### BUG-018: volatile + 忙等待 ⚠️

**原位置**: `src/libs/qsmedia/Api/private/IAudioPlayback_p.h:29`（`volatile int state;`，SDLPlayback 通过继承使用）
**新位置**: `src/audio/src/AudioPlayback.cpp`

```cpp
// AudioPlayback::Impl
struct AudioPlayback::Impl {
    std::atomic<int> state{AudioPlayback::Stopped};
    std::mutex stateMutex;
    std::condition_variable stateCV;
    // ...

    void waitForStateChange(int fromState) {
        std::unique_lock<std::mutex> lock(stateMutex);
        stateCV.wait(lock, [&] { return state.load() != fromState; });
    }

    void setState(int newState) {
        state.store(newState);
        stateCV.notify_all();
    }
};

// SDL poll 线程中:
// 原: while (state == Stopped) {}   ← 100% CPU
// 新:
d->waitForStateChange(Stopped);
```

### BUG-019: mutex RAII ⚠️

```cpp
// 原:
// mutex.lock();
// ... 代码 ...
// mutex.unlock();

// 修复:
{
    std::lock_guard<std::mutex> guard(d->pcmMutex);
    // ... 代码 ...
}  // 自动解锁
```

### BUG-020: 空设备列表 ⚠️

```cpp
// 原: q->setDevice(devices.front());
// 修复:
if (!devices.isEmpty()) {
    q->setDevice(devices.front());
}
```

### CQ-001: PlayWidget 合并

由 `src/widgets/PlayWidget` 统一实现，详见 02-module-spec.md §3.1。

---

## Medium (18)

### BUG-003: F0Widget switch break ✅

**已修复**: `src/apps/SlurCutter/gui/F0Widget.cpp:1063` — case Glide 块末尾添加 `break;`。

**新位置**: `src/apps/SlurCutter/F0Widget/F0Widget.cpp`

```cpp
case Glide: {
    // ... 原逻辑 ...
    setCursor(Qt::ArrowCursor);
    break;   // ← 新增
}
case None:
    break;
```

### BUG-004: 硬编码路径 ✅

**已修复**: LyricFAWindow.cpp 3 处、HubertFAWindow.cpp 1 处硬编码开发路径已清除为空字符串。

**新位置**: `src/apps/pipeline/lyricfa/LyricFAPage.cpp`, `src/apps/pipeline/hubertfa/HubertFAPage.cpp`

所有硬编码开发路径 → `""`（空字符串）：
- LyricFA 中 3 处 `R"(D:\python\LyricFA\...)"` → `""`
- HubertFA 中 1 处 `R"(C:\Users\99662\Desktop\...)"` → `""`

### BUG-005: 中文正则 ✅

**已修复**: `src/apps/LyricFA/util/ChineseProcessor.cpp:12` — `"[^[\\u4e00-\\u9fa5a]]"` → `"[^\\u4e00-\\u9fa5]"`。

**新位置**: `src/apps/pipeline/lyricfa/ChineseProcessor.cpp`

```cpp
// 原: "[^[\\u4e00-\\u9fa5a]]"
// 修复:
static const QRegularExpression nonChinese("[^\\u4e00-\\u9fa5]");
```

### BUG-007: HFA 内存泄漏 ⏳

**新位置**: `src/infer/hubert-infer/src/Hfa.cpp`

所有裸指针 → `std::unique_ptr`。详见 02-module-spec.md §4.3。

### BUG-010: 未初始化指针 ⏳

随 PlayWidget 合并自动修复（新代码统一使用 `= nullptr` 初始化）。

### BUG-013: f0Values 空向量 ⏳

**新位置**: `src/apps/SlurCutter/F0Widget/NoteDataModel.cpp`

```cpp
void NoteDataModel::loadFromDsSentence(const QJsonObject &obj) {
    // ... 解析 f0_seq ...
    if (m_f0Values.isEmpty()) {
        // 设置安全默认值，避免 UB
        emit errorOccurred(tr("Empty F0 data in sentence"));
        return;
    }
    m_f0Min = *std::min_element(m_f0Values.begin(), m_f0Values.end());
    m_f0Max = *std::max_element(m_f0Values.begin(), m_f0Values.end());
}
```

### BUG-014: PlayWidget 已知崩溃 ⏳

随 BUG-001 修复方案一并解决（`m_valid` 守卫阻止在未初始化时操作）。

### BUG-019/020: SDL mutex 和空列表

见上方 High 部分。

### BUG-021: HfaModel 空 session ⏳

**新位置**: `src/infer/hubert-infer/src/HfaModel.cpp`

```cpp
bool HfaModel::forward(...) const {
    if (!m_model_session) {
        msg = "Model session not initialized";
        return false;
    }
    // ... 原逻辑 ...
}
```

### BUG-022: FunAsr 资源泄漏 ⏳

**新位置**: `src/infer/funasr/src/paraformer_onnx.cpp`

```cpp
class ModelImp {
    std::unique_ptr<FeatureExtract> fe;
    std::unique_ptr<Ort::Session> m_session;
    std::unique_ptr<Vocab> vocab;
    // 构造函数中 make_unique，异常安全
};
```

### BUG-024: splitPitch 越界 ⏳

**新位置**: `src/apps/SlurCutter/F0Widget/NoteDataModel.cpp`

```cpp
auto splitPitch = notePitch.split(QRegularExpression(R"((\+|\-))"));
note.pitch = MusicUtils::noteNameToMidiNote(splitPitch[0]);
note.cents = (splitPitch.size() > 1) ? splitPitch[1].toDouble() : 0.0;
```

### CQ-002: F0Widget 拆分

详见 01-architecture.md §2.3 和 02-module-spec.md §5.3。

### CQ-003: i18n

Phase 1：所有用户可见字符串用 `tr()` 包裹：

```cpp
// 原: "添加文件"
// 新: tr("Add Files")

// 原: "Failed to load playback: %1!"
// 新: tr("Failed to load playback: %1!")
```

### CQ-008: 裸指针 → 智能指针

全局替换规则：

| 位置 | 原 | 新 |
|------|-----|-----|
| PlayWidget decoder/playback | `new` + 手动 delete | 构造函数内 new + 析构 null 检查 delete（因 Qt 生命周期） |
| HFA m_dictG2p | `new DictionaryG2P` | `std::make_unique<DictionaryG2P>` |
| HFA m_alignmentDecoder | `new AlignmentDecoder` | `std::make_unique<AlignmentDecoder>` |
| HFA m_nonLexicalDecoder | `new NonLexicalDecoder` | `std::make_unique<NonLexicalDecoder>` |
| FunAsr fe/session/vocab | `new Xxx` | `std::make_unique<Xxx>` |
| HfaModel m_model_session | `new Ort::Session` | `std::unique_ptr<Ort::Session>` |

注意：`PlayWidget` 中的 `m_playback` 用裸指针 + 手动管理（因为 SDL dispose 和 Qt 析构顺序需要精确控制），但确保析构函数 null 安全。

### CQ-009: 错误处理统一

详见 02-module-spec.md §1.3 ErrorHandling 约定。

---

## Low (15)

### BUG-008: QDir::count()

```cpp
// 原: dir.count()
// 新: dir.entryList(audioFilters, QDir::Files).count()
```

### BUG-011: #endif ✅

**已修复**: `src/apps/HubertFA/util/DictionaryG2P.h:24` — `#endif DICTIONARY_G2P_H` → `#endif // DICTIONARY_G2P_H`。

```cpp
// 原: #endif DICTIONARY_G2P_H
// 新: #endif // DICTIONARY_G2P_H
```

### BUG-012: GameInfer 错误提示

```cpp
// 修复 1: "is not exists" → "does not exist"
// 修复 2: .arg(text.toLocal8Bit().toStdString()) → .arg(text)
// 修复 3: QMessageBox::information → QMessageBox::critical
```

### BUG-023: ftell 截断 ⏳

**未修复**: `src/libs/FunAsr/src/util.cpp:15` — 仍为 `uint32_t nFileLen = ftell(fp);`，ftell 返回 `long`，`uint32_t` 截断会丢失错误标志 `-1` 且在 64 位系统上截断大文件大小。

```cpp
// 当前代码 (未修复):
uint32_t nFileLen = ftell(fp);

// 应修复为:
long pos = ftell(fp);
if (pos < 0) { fclose(fp); return nullptr; }
size_t nFileLen = static_cast<size_t>(pos);
```

### BUG-025: 类型双关 ⏳

**未修复**: `src/libs/FunAsr/src/FeatureExtract.cpp:110` — 仍为 `const float min_resol = *((float *) &val);`，应改为 `std::memcpy`。

```cpp
// 原: const float min_resol = *((float *) &val);
// 新:
float min_resol;
std::memcpy(&min_resol, &val, sizeof(float));
```

### CQ-004: foreach → 范围 for (8 处)

```cpp
// 原: foreach (auto f0, f0Seq)
// 新: for (const auto &f0 : f0Seq)

// 原: foreach (QString durStr, noteDur)
// 新: for (const QString &durStr : noteDur)
```

**精确位置（重构后）**：
1. `NoteDataModel.cpp` (原 F0Widget.cpp:147) — `foreach(auto f0, f0Seq)`
2. `PianoRollRenderer.cpp` (原 F0Widget.cpp:829,833) — `foreach` in paint
3. `SlurCutter/MainWindow.cpp` (原 MainWindow.cpp:247,356) — 2 处
4. `MinLabel/MainWindow.cpp` (原 MainWindow.cpp:464) — 1 处
5. `Common.cpp` (原 Common.cpp:67,205) — 2 处

### CQ-005: 循环内正则 → static

```cpp
// 原 (循环内):
// QRegularExpression re("...");

// 新:
static const QRegularExpression re("...");
```

### CQ-006: 拼写修正

| 原 | 新 | 位置 |
|-----|-----|------|
| `covertAction` | `convertAction` | MinLabel MainWindow.h/.cpp (4处) |
| `FaTread` | `LyricMatchTask` | 文件名 + 类名 (pipeline/lyricfa/, 9处) |

### CQ-007: 不雅注释

删除 `F0Widget.cpp:524` 的注释，替换为：
```cpp
// Merge the current slur note into the preceding note
```

### CQ-010: FunAsr 魔数

为关键魔数添加命名常量：
```cpp
constexpr int VOCAB_SIZE = 8404;
constexpr int FEATURE_DIM = 512;
constexpr int GLU_DIM_IN = 1024;
constexpr int GLU_DIM_OUT = 512;
```

### CQ-011: pcm_buffer 分配

```cpp
// 原: pcm_buffer.reset(new float[pcm_buffer_size * sizeof(float)]);
// 新: pcm_buffer.reset(new float[pcm_buffer_size]);
```

---

## 修复验证矩阵

| Bug ID | 自动测试 | 手动测试 |
|--------|----------|----------|
| BUG-001 | 构造 PlayWidget 后检查 m_valid | 拔掉音频设备启动 |
| BUG-003 | — | 在 F0Widget 中执行 Glide 操作后检查状态 |
| BUG-005 | `TestChineseProcessor` | 输入混合中英文 |
| BUG-006 | `TestSlicer::csvRoundTrip` | 导出/导入 CSV |
| BUG-013 | `TestNoteDataModel::emptyF0` | 加载空 f0_seq 的 .ds |
| BUG-015 | `TestFunAsr::missingModel` | 指定不存在的路径 |
| BUG-016 | `TestFunAsr::shortString` | 输入长度<3的字符串 |
| BUG-017 | 编译器检查虚析构 | — |
| BUG-018 | `TestAudioPlayback::stateTransitions` | CPU 使用率监控 |
| BUG-024 | `TestNoteDataModel::pureNoteName` | 加载 "C4" (无偏移) |

---

## v3.1 修复进度总结

### 已修复 ✅（7 项，直接在当前代码原位修复，无需等待重构）

| Bug ID | 严重度 | 修复文件 | 修复内容 |
|--------|--------|----------|----------|
| BUG-006 | Critical | `slicer/workthread.cpp:261` | capturedView 索引 0/1/2 → 1/2/3 |
| BUG-015 | Critical | `FunAsr/src/util.cpp` | loadparams/SaveDataFile 添加 fopen NULL 检查 |
| BUG-009 | High | `LyricFA/util/AsrThread.cpp:24` | QMessageBox → emit oneFailed 信号 |
| BUG-003 | Medium | `SlurCutter/gui/F0Widget.cpp:1063` | case Glide 补 break |
| BUG-004 | Medium | `LyricFAWindow.cpp` + `HubertFAWindow.cpp` | 4 处硬编码路径 → 空字符串 |
| BUG-005 | Medium | `LyricFA/util/ChineseProcessor.cpp:12` | 修正畸形正则表达式 |
| BUG-011 | Low | `HubertFA/util/DictionaryG2P.h:24` | `#endif` 补 `//` 注释 |

### 待重构时修复 ⏳（11 项，依赖架构重构或模块合并）

BUG-007, BUG-010, BUG-013, BUG-014, BUG-016, BUG-017, BUG-021, BUG-022, BUG-023, BUG-024, BUG-025

### 需设计评审 ⚠️（5 项，修复方案存在副作用风险）

| Bug ID | 风险说明 |
|--------|----------|
| BUG-001 | saveFile 返回 false 后静默切换文件可能导致数据丢失；建议失败时弹窗询问用户 |
| BUG-018 | volatile → atomic + CV 改动涉及 SDL 回调线程模型，需全面审查状态读写路径 |
| BUG-019 | SDL 回调中 mutex RAII 化需确认回调内无提前 return 分支遗漏 |
| BUG-020 | 空设备检查后需整条音频管线都能处理"无设备"状态，否则崩溃转移到下游 |
| CQ-011 | pcm_buffer_size 语义需确认是"浮点数个数"而非"字节数"，否则修复方向相反 |
