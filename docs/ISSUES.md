# DiffSinger Dataset Tools — Issues & Improvement Tracker

**Version**: 3.0  
**Date**: 2026-04-26  
**审查方法**: 全量源码逐文件审查 + 架构分析 + 修复方案可行性复核  

本文档记录通过深度代码审查发现的所有 Bug、代码质量问题和功能改进建议，包含精确代码上下文、影响分析和经过可行性验证的修复方案。

> **v3.0 变更说明**: 对 v2.0 所有修复方案进行了逐一复核，修正了 3 个方案缺陷，新增 10 个遗漏问题。总计 **45 个问题** (v2.0 为 29 个)。

---

## Bug (缺陷)

### BUG-001: exit(-1) 导致非正常退出 [严重]

**位置 (共 5 处)**:
- `src/apps/MinLabel/gui/PlayWidget.cpp:220`
- `src/apps/MinLabel/gui/MainWindow.cpp:242`
- `src/apps/MinLabel/gui/MainWindow.cpp:255`
- `src/apps/MinLabel/gui/MainWindow.cpp:499`
- `src/apps/SlurCutter/gui/PlayWidget.cpp:243`

**代码上下文**:

```cpp
// PlayWidget.cpp:203-221 (MinLabel & SlurCutter 完全相同)
void PlayWidget::initPlugins() {
    decoder = new FFmpegDecoder();
    playback = new SDLPlayback();

    if (!playback->setup(QsMedia::PlaybackArguments{44100, 2, 1024})) {
        QMessageBox::critical(this, qApp->applicationName(),
                              QString("Failed to load playback: %1!").arg("SDLPlayback"));
        goto out;
    }
    playback->setDecoder(decoder);
    return;

out:
    uninitPlugins();
    exit(-1);   // ← 强制终止进程
}
```

**影响分析**:
- `exit(-1)` 直接调用 C 运行时退出，**跳过所有栈上对象的析构函数**
- Qt 事件循环不会正常退出，`QApplication` 析构器不执行
- 已打开的文件句柄可能不被刷新，导致**数据截断或损坏**
- SDL 音频设备不释放，可能导致下次启动音频设备占用
- 用户正在编辑的其他文件的未保存数据将全部丢失

**修复方案** (v3.0 修正):

> **v2.0 方案缺陷**: 原方案建议 `initPlugins()` 失败时 `return` + `setEnabled(false)`。但经复核发现：`initPlugins()` 在构造函数中调用 (line 85)，返回后构造函数会继续执行到 line 133-136 的 `connect(playback, ...)` ——此时 `playback` 已被 `uninitPlugins()` delete，导致**悬挂指针崩溃**。且析构函数 (line 142) 无条件调用 `playback->dispose()` 也会崩溃。所以不能简单 return。

```cpp
// PlayWidget — 正确修复方案: 引入 pluginsValid 标志位
class PlayWidget : public QWidget {
    bool m_pluginsValid = false;   // ← 新增
    IAudioDecoder *decoder{};
    IAudioPlayback *playback{};
    ...
};

void PlayWidget::initPlugins() {
    decoder = new FFmpegDecoder();
    playback = new SDLPlayback();

    if (!playback->setup(QsMedia::PlaybackArguments{44100, 2, 1024})) {
        QMessageBox::critical(this, qApp->applicationName(),
            QString("Failed to load playback: %1!").arg("SDLPlayback"));
        delete playback; playback = nullptr;
        delete decoder;  decoder = nullptr;
        return;  // m_pluginsValid 保持 false
    }
    playback->setDecoder(decoder);
    m_pluginsValid = true;
}

// 构造函数中: 将 connect 移到 initPlugins 成功后
PlayWidget::PlayWidget(QWidget *parent) : QWidget(parent) {
    // ... UI 布局代码 ...
    initPlugins();
    if (m_pluginsValid) {
        connect(playback, &IAudioPlayback::stateChanged, this, &PlayWidget::_q_playStateChanged);
        connect(playback, &IAudioPlayback::deviceChanged, this, &PlayWidget::_q_audioDeviceChanged);
        connect(playback, &IAudioPlayback::deviceAdded,   this, &PlayWidget::_q_audioDeviceAdded);
        connect(playback, &IAudioPlayback::deviceRemoved,  this, &PlayWidget::_q_audioDeviceRemoved);
        reloadDevices();
    } else {
        playButton->setEnabled(false);
        stopButton->setEnabled(false);
    }
}

// 析构函数: 添加 null 检查
PlayWidget::~PlayWidget() {
    if (playback) playback->dispose();
    if (decoder)  decoder->close();
    delete playback;
    delete decoder;
}

// 所有使用 decoder/playback 的方法添加守卫
void PlayWidget::setPlaying(bool playing) {
    if (!m_pluginsValid) return;
    // ... 原有逻辑
}

void PlayWidget::openFile(const QString &filename) {
    if (!m_pluginsValid) return;
    // ... 原有逻辑
}
```

```cpp
// MainWindow::saveFile() — 正确修复方案: 返回 bool + 调用者处理
// v2.0 方案缺陷: saveFile 是 void，调用者不检查返回值;
// 递归 retry 可能 stack overflow

bool MainWindow::saveFile(const QString &filename) {  // void → bool
    ...
    if (!writeJsonFile(jsonFilePath, writeData)) {
        QMessageBox::critical(this, QApplication::applicationName(),
            QString("Failed to write to file %1").arg(jsonFilePath));
        return false;  // 替代 exit(-1)
    }
    ...
    if (!labFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QApplication::applicationName(),
            QString("Failed to write to file %1").arg(labFilePath));
        return false;  // 替代 exit(-1)
    }
    ...
    return true;
}

// 调用者 (析构函数/树切换) 可选择忽略失败:
// _q_treeCurrentChanged() 中: saveFile(lastFile); // 失败不影响导航
// 析构函数中: saveFile(lastFile); // 尽力保存，失败不阻塞退出
```

**工作量估计**: 4h (需修改构造函数、析构函数及所有 ~15 个方法添加守卫)  
**标签**: `bug`, `critical`, `MinLabel`, `SlurCutter`

---

### BUG-002: goto 语句导致代码流程混乱 [中等]

**位置**:
- `src/apps/MinLabel/gui/PlayWidget.cpp:213`
- `src/apps/SlurCutter/gui/PlayWidget.cpp:236`

**分析**: 与 BUG-001 合并修复，使用早期 return 替代 goto + exit。

**标签**: `bug`, `code-quality`, `MinLabel`, `SlurCutter`

---

### BUG-003: F0Widget mouseReleaseEvent switch 缺少 break [中等]

**位置**: `src/apps/SlurCutter/gui/F0Widget.cpp:1054-1067`

**代码上下文**:

```cpp
            case Glide: {
                const int deltaPitch = std::round(pitchOnWidgetY(event->position().y())) - draggingNoteStartPitch;
                if (deltaPitch == 0)
                    setDraggedNoteGlide(GlideStyle::None);
                else if (deltaPitch > 0)
                    setDraggedNoteGlide(GlideStyle::Up);
                else if (deltaPitch < 0)
                    setDraggedNoteGlide(GlideStyle::Down);
                setCursor(Qt::ArrowCursor);
            }   // ← 缺少 break! Fall-through 到 case None

            case None:
                break;
```

**影响**: 当前功能无害 (`case None` 仅 `break`)，但是潜在炸弹。

**修复**: 在 `setCursor(Qt::ArrowCursor);` 后添加 `break;`。  
**工作量**: 5min  
**标签**: `bug`, `medium`, `SlurCutter`

---

### BUG-004: 开发者硬编码路径残留 [中等]

**位置 (4 处)**: `LyricFAWindow.cpp:31,38,45`, `HubertFAWindow.cpp:75`

**修复**: 所有硬编码路径替换为空字符串 `""`。  
**工作量**: 30min  
**标签**: `bug`, `cleanup`, `LyricFA`, `HubertFA`

---

### BUG-005: 中文字符过滤正则表达式错误 [中等]

**位置**: `src/apps/LyricFA/util/ChineseProcessor.cpp:12`

**修复**: `"[^[\\u4e00-\\u9fa5a]]"` → `"[^\\u4e00-\\u9fa5]"`。并改为 `static const`。  
**工作量**: 10min  
**标签**: `bug`, `LyricFA`

---

### BUG-006: CSV 标记解析正则捕获组索引错误 [严重]

**位置**: `src/apps/AudioSlicer/slicer/workthread.cpp:261-265`

**影响**: `capturedView(0)` 返回整个匹配 (含 `:` 和 `.`)，`toLongLong` 必定失败。**Audacity CSV 时间戳解析功能完全不可用**。调用者 `loadCSVMarkers` (line 393/395) 会收到 `MarkerError::FormatError`。

**修复**: `capturedView(0/1/2)` → `capturedView(1/2/3)`。  
**工作量**: 10min  
**标签**: `bug`, `critical`, `AudioSlicer`

---

### BUG-007: HFA 类多指针内存泄漏 [中等]

**位置**: `src/apps/HubertFA/util/Hfa.h:34-36`

**修复方案** (v3.0 复核确认可行):
- `m_dictG2p` map 通过 `find()->second` 访问 (line 110/112)，`unique_ptr` 可正常工作
- `m_alignmentDecoder` 和 `m_nonLexicalDecoder` 仅在 `recognize()` 内部使用，无外部所有权转移
- HFA 类未被拷贝/移动

```cpp
std::map<std::string, std::unique_ptr<DictionaryG2P>> m_dictG2p;
std::unique_ptr<AlignmentDecoder> m_alignmentDecoder;
std::unique_ptr<NonLexicalDecoder> m_nonLexicalDecoder;

// 赋值: m_dictG2p[language] = std::make_unique<DictionaryG2P>(...);
// 访问: auto it = m_dictG2p.find(language); it->second->convert(...);
```

**工作量**: 30min  
**标签**: `bug`, `memory-leak`, `HubertFA`

---

### BUG-008: QDir::count() 包含 . 和 .. 导致进度不准 [低]

**位置**: `src/apps/MinLabel/gui/MainWindow.cpp:417-425`

**修复**: 使用 `QDir::entryList(audioFilters, QDir::Files).count()` 替代 `QDir::count()`。  
**工作量**: 15min  
**标签**: `bug`, `minor`, `MinLabel`

---

### BUG-009: 工作线程中调用 QMessageBox [中等]

**位置**: `src/apps/LyricFA/util/AsrThread.cpp:24`

**代码**: `QMessageBox::critical(nullptr, ...)` 在 `run()` (工作线程) 中调用，用于文件写入失败场景 (line 24)。

**修复**: 替换为 `emit oneFailed(m_filename, QString::fromStdString(msg)); return;`  
**工作量**: 30min  
**标签**: `bug`, `thread-safety`, `LyricFA`

---

### BUG-010: 未初始化指针 [中等]

**位置**: `src/apps/SlurCutter/gui/PlayWidget.h:35-36`

**对比**:
- MinLabel: `IAudioDecoder *decoder{};` ← 正确
- SlurCutter: `IAudioDecoder *decoder;` ← **未初始化**

**修复**: 添加 `{}` 值初始化器。  
**标签**: `bug`, `SlurCutter`

---

### BUG-011: #endif 语法不规范 [低]

**位置**: `src/apps/HubertFA/util/DictionaryG2P.h:24`  
**修复**: `#endif DICTIONARY_G2P_H` → `#endif // DICTIONARY_G2P_H`  
**标签**: `bug`, `minor`, `HubertFA`

---

### BUG-012: GameInfer 错误提示问题 (3处) [低]

**位置**: `src/apps/GameInfer/gui/MainWidget.cpp`

| 行 | 问题 | 修复 |
|---|---|---|
| 571 | `"Audio file is not exists"` | → `"Audio file does not exist"` |
| 572 | `.arg(m_wavPathLineEdit->text().toLocal8Bit().toStdString())` — 传 std::string 给 QString::arg() | → `.arg(m_wavPathLineEdit->text())` |
| 585 | `QMessageBox::information(..."Fail"...)` 用于失败 | → `QMessageBox::critical(...)` |

**工作量**: 10min  
**标签**: `bug`, `minor`, `GameInfer`

---

### BUG-013: f0Values 空向量未定义行为 [中等]

**位置**: `src/apps/SlurCutter/gui/F0Widget.cpp:255-263`

**代码上下文**:

```cpp
// setDsSentenceContent() 中:
// lines 255-256: std::min_element / std::max_element on f0Values
// line 263: f0Values.first()
// 如果 f0_seq 为空字符串，f0Values 为空向量
```

**影响**: `std::min_element` / `std::max_element` 对空范围返回 end 迭代器，解引用是 UB。`f0Values.first()` 对空 QVector 也是 UB → 崩溃。

**修复**: 在访问前添加 `if (f0Values.isEmpty()) { setErrorStatusText("Empty F0 data"); return; }`  
**工作量**: 15min  
**标签**: `bug`, `medium`, `SlurCutter`

---

### BUG-014: PlayWidget 已知崩溃注释未修复 [中等]

**位置**: `src/apps/SlurCutter/gui/PlayWidget.cpp:209`

**描述**: 注释 `"Sets before media is initialized, will crash"` 表明已知问题。应与 BUG-001 一并修复。

**标签**: `bug`, `known-issue`, `SlurCutter`

---

### BUG-015: FunAsr fopen 返回值未检查 (NULL 解引用) [新发现] [严重]

**位置**:
- `src/libs/FunAsr/src/util.cpp:7-17` (`loadparams` 函数)
- `src/libs/FunAsr/src/util.cpp:34-38` (`SaveDataFile` 函数)

**代码上下文**:

```cpp
// util.cpp:7-20  loadparams
fp = fopen(filename, "rb");
fseek(fp, 0, SEEK_END);     // ← fp 可能为 NULL → 段错误
uint32_t nFileLen = ftell(fp);
...
fclose(fp);

// util.cpp:36-38  SaveDataFile
fp = fopen(filename, "wb+");
fwrite(data, 1, len, fp);   // ← fp 可能为 NULL
fclose(fp);
```

**影响**: 若模型文件不存在或路径错误，`fopen` 返回 NULL，后续所有 `fseek`/`ftell`/`fread`/`fclose` 均为 NULL 指针解引用，导致**段错误崩溃**。

**修复**:

```cpp
fp = fopen(filename, "rb");
if (!fp) {
    LOG("Failed to open file: %s\n", filename);
    return nullptr;  // 或返回错误码
}
```

**工作量**: 15min  
**标签**: `bug`, `critical`, `FunAsr`

---

### BUG-016: FunAsr str2int 越界访问 [新发现] [高]

**位置**: `src/libs/FunAsr/src/Vocab.cpp:43-45`

**代码上下文**:

```cpp
int str2int(const std::string &str) {  // ← FunAsr 命名空间下的自由函数，非 Vocab 成员
    const char *ch_array = str.c_str();
    if (((ch_array[0] & 0xf0) != 0xe0) || ((ch_array[1] & 0xc0) != 0x80) || ((ch_array[2] & 0xc0) != 0x80))
    //     ↑ 未检查 str.size() >= 3
```

**影响**: 若输入字符串长度 < 3，`ch_array[1]` 和 `ch_array[2]` 越界读取。

**修复**: 添加 `if (str.size() < 3) return -1;` 前置检查。  
**工作量**: 10min  
**标签**: `bug`, `high`, `FunAsr`

---

### BUG-017: WaveStream 非虚析构函数 [新发现] [高]

**位置**: `src/libs/qsmedia/NAudio/WaveStream.h:10-13`

**代码上下文**:

```cpp
class QSMEDIA_API WaveStream {
public:
    WaveStream();
    ~WaveStream();                                          // ← 非虚!
    virtual WaveFormat Format() const = 0;                  // 纯虚方法
    virtual int Read(char *buffer, int offset, int count) = 0;
```

**影响**: 通过 `WaveStream*` 指针 delete 派生类对象是**未定义行为**。`IAudioDecoder` 继承自 `WaveStream`，而 `FFmpegDecoder` 继承自 `IAudioDecoder`，所以这是真实风险。

**修复**: `~WaveStream();` → `virtual ~WaveStream();`  
**工作量**: 5min  
**标签**: `bug`, `high`, `qsmedia`

---

### BUG-018: volatile 误用为线程同步 + 忙等待 [新发现] [高]

**位置**:
- `src/libs/qsmedia/Api/private/IAudioPlayback_p.h:28` — `volatile int state;`
- `src/libs/sdlplayback/private/SDLPlayback_p.cpp:117-124`

**代码上下文**:

```cpp
// IAudioPlayback_p.h:28
volatile int state;   // ← volatile ≠ atomic！不是线程安全的

// SDLPlayback_p.cpp:117-118 (SDL 轮询线程)
while (state == IAudioPlayback::Stopped) {
}  // ← 无条件忙等待！100% CPU 占用
// ...
while (state == IAudioPlayback::Playing) {
}  // ← 同上
```

**影响**:
1. `volatile` 在 C++ 中**不提供线程同步语义**，编译器可以重排读写 → 数据竞争 (UB)
2. 忙等待循环消耗 100% CPU 核心

**修复**:

```cpp
// IAudioPlayback_p.h
std::atomic<int> state{IAudioPlayback::Stopped};

// SDLPlayback_p.cpp — 用条件变量替代忙等待
std::condition_variable stateCV;
std::mutex stateMutex;

// 等待状态变化:
{
    std::unique_lock<std::mutex> lock(stateMutex);
    stateCV.wait(lock, [&] { return state != IAudioPlayback::Stopped; });
}

// 状态变更时通知:
state.store(IAudioPlayback::Playing);
stateCV.notify_all();
```

**工作量**: 2h  
**标签**: `bug`, `high`, `sdlplayback`, `thread-safety`

---

### BUG-019: SDL mutex 手动 lock/unlock 无 RAII [新发现] [中等]

**位置**: `src/libs/sdlplayback/private/SDLPlayback_p.cpp:222,244`

**代码**: `mutex.lock(); ... mutex.unlock();` — 若中间代码抛异常或提前 return，mutex 永不释放 → 死锁。

**修复**: 替换为 `std::lock_guard<std::mutex> guard(mutex);`  
**工作量**: 15min  
**标签**: `bug`, `medium`, `sdlplayback`

---

### BUG-020: SDL devices.front() 空列表崩溃 [新发现] [中等]

**位置**: `src/libs/sdlplayback/private/SDLPlayback_p.cpp:107-108`

**代码**: `q->setDevice(devices.front());` — 若无音频设备，`devices` 为空，`.front()` 为 UB。

**修复**: `if (!devices.isEmpty()) q->setDevice(devices.front());`  
**工作量**: 5min  
**标签**: `bug`, `medium`, `sdlplayback`

---

### BUG-021: HfaModel m_model_session 空指针解引用 [新发现] [中等]

**位置**: `src/apps/HubertFA/util/HfaModel.cpp:66,94`

**描述**: 构造函数 (line 49-57) 中 `new Ort::Session` 若抛异常被 catch 后 `m_model_session` 为 null，但 `forward()` (line 66) 未检查直接调用 `m_model_session->Run()`。

**修复**: `if (!m_model_session) { msg = "Model not loaded"; return {}; }`  
**工作量**: 10min  
**标签**: `bug`, `medium`, `HubertFA`

---

### BUG-022: FunAsr ModelImp 构造函数资源泄漏 [新发现] [中等]

**位置**: `src/libs/FunAsr/src/paraformer_onnx.cpp:15-26`

**代码上下文**:

```cpp
ModelImp::ModelImp(...) {
    fe = new FeatureExtract(3);
    // ...
    m_session = new Ort::Session(env, ...);  // 可能抛异常 → fe 泄漏
    vocab = new Vocab(vocab_path...);        // 可能抛异常 → fe + m_session 泄漏
}
```

**修复**: 使用 `std::unique_ptr` 管理 `fe`, `m_session`, `vocab`。  
**工作量**: 30min  
**标签**: `bug`, `memory-leak`, `FunAsr`

---

### BUG-023: FunAsr ftell→uint32_t 类型截断 [新发现] [低]

**位置**: `src/libs/FunAsr/src/util.cpp:12`

**代码**: `uint32_t nFileLen = ftell(fp);` — `ftell` 返回 `long`，错误时为 -1，转换为 uint32_t 变成 ~4GB。

**修复**: `long pos = ftell(fp); if (pos < 0) return nullptr; size_t nFileLen = static_cast<size_t>(pos);`  
**标签**: `bug`, `low`, `FunAsr`

---

### BUG-024: F0Widget splitPitch 越界访问 [新发现] [中等]

**位置**: `src/apps/SlurCutter/gui/F0Widget.cpp:222-224`

**代码上下文**:

```cpp
auto splitPitch = notePitch.split(QRegularExpression(R"((\+|\-))"));
note.pitch = NoteNameToMidiNote(splitPitch[0]);
note.cents = splitPitch[1].toDouble();   // ← splitPitch 可能只有 1 个元素
```

**影响**: 如果音符名不含 `+` 或 `-` (如纯 `"C4"`)，split 结果只有 1 个元素，`splitPitch[1]` 越界。

**修复**: `note.cents = (splitPitch.size() > 1) ? splitPitch[1].toDouble() : 0.0;`  
**工作量**: 5min  
**标签**: `bug`, `medium`, `SlurCutter`

---

### BUG-025: FunAsr 类型双关 (Type Punning) UB [新发现] [低]

**位置**: `src/libs/FunAsr/src/FeatureExtract.cpp:109-110`

**代码**:

```cpp
int val = 0x34000000;
const float min_resol = *((float *) &val);  // ← 严格别名规则违反 → UB
```

**修复**: `float min_resol; std::memcpy(&min_resol, &val, sizeof(float));`  
**标签**: `bug`, `low`, `FunAsr`

---

---

## 代码质量问题

### CQ-001: PlayWidget 代码重复 (~75%) [高]

**位置**:
- `src/apps/MinLabel/gui/PlayWidget.h/.cpp` (~342 行)
- `src/apps/SlurCutter/gui/PlayWidget.h/.cpp` (~390 行)

**逐方法对比** — 15 个方法 100% 相同，6 个方法有差异 (SlurCutter 添加范围播放)。

**修复方案** (v3.0 修正):

> **v2.0 方案遗漏**: 原方案未考虑 `uninitPlugins()` 的 null 安全问题，以及基类析构函数需要 null 检查。v3.0 方案整合了 BUG-001 修复。

```
新增: src/libs/play-widget-base/
├── CMakeLists.txt
├── include/PlayWidgetBase.h
└── src/PlayWidgetBase.cpp

关键设计:
1. 基类包含 m_pluginsValid 标志位 (来自 BUG-001 修复)
2. uninitPlugins() 添加 null 检查并置空指针
3. 析构函数先检查 null 再调用 dispose/close
4. 差异化方法通过 virtual 机制覆写:
   - virtual void onPlayClicked()
   - virtual void onStopClicked()
   - virtual void onSliderReleased(double percentage)
   - virtual void onPlaybackStateChanged(bool isPlaying)
   - virtual void updateSliderDisplay()

void PlayWidgetBase::uninitPlugins() {
    if (playback) { delete playback; playback = nullptr; }
    if (decoder)  { delete decoder;  decoder = nullptr; }
    m_pluginsValid = false;
}
```

**工作量**: 6h (含 BUG-001 修复)  
**标签**: `code-quality`, `refactoring`, `high-priority`

---

### CQ-002: F0Widget 巨型类 (1097 行) [中等]

**位置**: `src/apps/SlurCutter/gui/F0Widget.h/.cpp`

**拆分方案** (v3.0 同 v2.0，复核确认可行):
- `MusicUtilities` — 静态工具 (~60行)
- `NoteDataModel` — 数据管理 (~250行)
- `PianoRollRenderer` — 渲染逻辑 (~320行)
- `F0Widget` — 轻量组合器 + 输入处理 (~160行)

**工作量**: 8h  
**标签**: `code-quality`, `refactoring`

---

### CQ-003: 全项目零国际化 (i18n) [中等]

**整个项目没有使用 `tr()` 包裹任何 UI 字符串**。中文和英文硬编码混合。AsyncTaskWindow/GameInfer 全中文，MinLabel/SlurCutter/AudioSlicer 全英文。

**修复**: 分阶段: Phase 1 用 `tr()` 包裹 → Phase 2 `lupdate` 提取 .ts → Phase 3 翻译 .qm  
**标签**: `code-quality`, `i18n`

---

### CQ-004: 弃用的 Qt foreach 宏 (8 处) [低]

**精确位置**: F0Widget.cpp:147,829,833; SlurCutter/MainWindow.cpp:247,356; MinLabel/MainWindow.cpp:464; Common.cpp:67,205

**注意**: line 147 `foreach (auto f0, f0Seq)` 按值拷贝每个 double; line 356 `foreach (QString durStr, noteDur)` 按值拷贝 QString。

**标签**: `code-quality`, `modernization`

---

### CQ-005: QRegularExpression 在循环中创建 [低]

**位置**: `MainWindow.cpp:485,492,494` (labToJson 内循环)。对比 `saveFile()` 正确使用 `static`。  
**标签**: `code-quality`, `performance`

---

### CQ-006: 拼写错误 (2 处系统性) [低]

- **"Covert" → "Convert"**: 4 处 `covertAction` (含成员变量，需同步修改 `MainWindow.h` 声明) + 2 处 `covertNum` (含成员变量 `TextWidget.h:31`，需同步修改头文件声明)
- **"Tread" → "Thread"**: 9 处 (含文件名 `FaTread.h/cpp`，需 `git mv` + 全局替换 + CMakeLists 更新)

**v3.0 补充**: `covertAction` 是**成员变量** (跨多方法使用)，重命名需更新 `MainWindow.h` 中的声明。FaTread 继承 QObject 有 `Q_OBJECT` 宏，重命名后 MOC 自动适配无需手动处理。

**工作量**: 30min  
**标签**: `code-quality`, `typo`

---

### CQ-007: 不雅注释 [低]

**位置**: `F0Widget.cpp:524` — 不适当注释。替换为技术说明。  
**标签**: `code-quality`, `cleanup`

---

### CQ-008: 原始 new/delete 无 RAII [中等]

**全项目普遍使用裸指针**。关键风险点:
- `PlayWidget` 的 `decoder`/`playback`
- `HFA` 的 3 个指针 (BUG-007)
- `FunAsr::ModelImp` 的 `fe`/`m_session`/`vocab` (BUG-022)

**标签**: `code-quality`, `modernization`

---

### CQ-009: 错误处理模式不一致 [中等]

| 应用 | 模式 | 问题 |
|---|---|---|
| MinLabel | `exit(-1)` | 非正常退出 |
| SlurCutter | `exit(-1)` | 同上 |
| AudioSlicer | `QMessageBox + return` | **最佳实践** |
| LyricFA | 工作线程 `QMessageBox` | **线程不安全** |
| HubertFA | `signal + slot` | 正确 |
| GameInfer | `QMetaObject::invokeMethod` | 正确 |

**标签**: `code-quality`, `architecture`

---

### CQ-010: FunAsr 硬编码魔数 [新发现] [低]

**位置**: `src/libs/FunAsr/src/` 多处

| 位置 | 魔数 | 含义 |
|---|---|---|
| `util.cpp:48,53` | `512` | 特征维度 |
| `paraformer_onnx.cpp:100` | `8404` | 词表大小 |
| `util.cpp:158` | `1024`, `512` | GLU 维度 |
| `FeatureExtract.cpp:119-319` | 200行 hardcoded | mel 滤波器组系数 |

**标签**: `code-quality`, `maintainability`

---

### CQ-011: SDLPlayback pcm_buffer 过度分配 [新发现] [低]

**位置**: `src/libs/sdlplayback/private/SDLPlayback_p.cpp:72`

**代码**: `pcm_buffer.reset(new float[pcm_buffer_size * sizeof(float)]);` — 分配了 `N * 4` 个 float，但只使用 `N` 个。`* sizeof(float)` 是多余的 (浪费 4x 内存)。

**修复**: `pcm_buffer.reset(new float[pcm_buffer_size]);`  
**标签**: `code-quality`, `sdlplayback`

---

---

## 功能改进建议

### FEAT-001: 撤销/重做功能 [高]

**位置**: `src/apps/SlurCutter/gui/F0Widget.h`

**设计**: `QUndoStack` + `QUndoCommand` 模式，为 7 种编辑操作各实现一个 Command 类。  
**工作量**: 6h  
**标签**: `feature`, `high-priority`, `SlurCutter`

---

### FEAT-002: 批量重采样工具 [中等]

**方案**: 基于 `AsyncTaskWindow` + `audio-util` 创建新应用。  
**工作量**: 4h  
**标签**: `feature`, `enhancement`

---

### FEAT-003: AudioSlicer 参数校验 [中等]

**校验**: `minInterval <= minLength`, `hopSize <= minInterval`, `threshold < 0`。  
**工作量**: 1h  
**标签**: `feature`, `usability`, `AudioSlicer`

---

### FEAT-004: LyricFA 线程安全改进 [中等]

**位置**: `src/apps/LyricFA/util/Asr.cpp:73-74`

**修复方案** (v3.0 复核确认):
- `recognize()` 不递归调用自身，简单 `std::mutex` 不会死锁
- `recognize()` 标记为 `const` 但通过指针修改 model 状态 → mutex 需要 `mutable`

```cpp
class Asr {
    mutable std::mutex m_mutex;
};
bool Asr::recognize(...) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    // ...
}
```

**标签**: `feature`, `robustness`, `LyricFA`

---

### FEAT-005: 跨平台 CI/CD [低]

添加 macOS + Linux GitHub Actions workflow。  
**标签**: `feature`, `ci-cd`

---

### FEAT-006: 单元测试覆盖不足 [高]

**P0 缺失测试**: SequenceAligner, Slicer, AlignmentDecoder  
**P1 缺失测试**: DictionaryG2P, ChineseProcessor, MusicUtilities  
**工作量**: 16h (P0)  
**标签**: `feature`, `testing`, `high-priority`

---

### FEAT-007: 配置系统统一 [低]

3 个应用有 INI 持久化，3 个没有。键名风格不统一。  
**标签**: `feature`, `architecture`

---

### FEAT-008: 结构化日志系统 [低]

引入 `QLoggingCategory` 分级日志。  
**标签**: `feature`, `observability`

---

### FEAT-009: PlayWidget 已知崩溃修复 [中等]

**位置**: `SlurCutter/PlayWidget.cpp:209`。与 BUG-001/BUG-014 合并修复。  
**标签**: `feature`, `robustness`, `SlurCutter`

---

## 优先级汇总

### Critical (立即修复)

| ID | 标题 | 影响 |
|---|---|---|
| BUG-001 | exit(-1) 非正常退出 (5处) | 数据丢失风险 |
| BUG-006 | CSV 解析索引错误 | 功能完全不可用 |
| BUG-015 | FunAsr fopen 空指针解引用 (2处) | 段错误崩溃 |

### High (本迭代修复)

| ID | 标题 | 影响 |
|---|---|---|
| BUG-009 | 工作线程调用 QMessageBox | 崩溃/冻结风险 |
| BUG-016 | Vocab str2int 越界访问 | 越界读取 |
| BUG-017 | WaveStream 非虚析构函数 | 通过基类指针 delete 派生类 = UB |
| BUG-018 | volatile 误用 + 忙等待 | 数据竞争 UB + 100% CPU |
| CQ-001 | PlayWidget 75% 代码重复 | 维护成本 |
| FEAT-001 | 撤销/重做功能 | 用户体验核心缺陷 |
| FEAT-006 | 单元测试覆盖 | 代码质量保障 |

### Medium (计划修复)

| ID | 标题 |
|---|---|
| BUG-003 | switch 缺少 break |
| BUG-004 | 开发者硬编码路径 (4处) |
| BUG-005 | 中文过滤正则错误 |
| BUG-007 | HFA 内存泄漏 (3处) |
| BUG-010 | 未初始化指针 |
| BUG-013 | f0Values 空向量 UB |
| BUG-014 | 已知崩溃注释未修复 |
| BUG-019 | mutex 手动 lock/unlock |
| BUG-020 | SDL devices.front() 空列表 |
| BUG-021 | HfaModel 空 session 解引用 |
| BUG-022 | FunAsr ModelImp 资源泄漏 |
| BUG-024 | splitPitch 越界访问 |
| CQ-002 | F0Widget 巨型类 |
| CQ-003 | 全项目零 i18n |
| CQ-008 | 原始 new/delete 无 RAII |
| CQ-009 | 错误处理模式不一致 |
| FEAT-003 | AudioSlicer 参数校验 |
| FEAT-004 | LyricFA 线程安全 |

### Low (有空就修)

| ID | 标题 |
|---|---|
| BUG-008 | 进度计算不准 |
| BUG-011 | #endif 语法 |
| BUG-012 | GameInfer 错误提示 (3处) |
| BUG-023 | ftell 类型截断 |
| BUG-025 | 类型双关 UB |
| CQ-004 | foreach 宏 (8处) |
| CQ-005 | 循环内编译正则 |
| CQ-006 | 拼写错误 (13处) |
| CQ-007 | 不雅注释 |
| CQ-010 | FunAsr 硬编码魔数 |
| CQ-011 | pcm_buffer 过度分配 |
| FEAT-002 | 批量重采样工具 |
| FEAT-005 | 跨平台 CI/CD |
| FEAT-007 | 配置系统统一 |
| FEAT-008 | 日志系统 |

---

## 修复工作量估算

| 优先级 | Issue 数量 | 预计总工时 |
|---|---|---|
| Critical | 3 | 4.5h |
| High | 7 | 28h |
| Medium | 18 | 20h |
| Low | 15 | 8h |
| **合计** | **43 项** | **~60.5h** |

---

## 附录: v3.0 修复方案修正记录

| Issue | v2.0 方案 | v3.0 修正 | 原因 |
|---|---|---|---|
| BUG-001 PlayWidget | `return` + `setEnabled(false)` | 引入 `m_pluginsValid` 标志 + null 守卫 | 原方案导致构造函数继续执行 `connect(playback, ...)` 悬挂指针崩溃 |
| BUG-001 saveFile | `return saveFile(filename)` 递归重试 | `return false` + 调用者处理 | 递归重试在持续错误时 stack overflow; saveFile 是 void 需改签名 |
| CQ-001 基类 | 未考虑 `uninitPlugins` null 安全 | `uninitPlugins` 添加 null 检查 + 置空 | `delete` 已删除的指针是 UB |
| BUG-012 | 仅报告英文语法 | 新增 `.arg(std::string)` 类型不匹配 | QString::arg 不接受 std::string |
| FEAT-009 | 标记为未完成 | 确认不存在 (TODO: INCOMPLETE 注释不在 F0Widget.cpp 中) | 误报，已更正描述为 PlayWidget 已知崩溃修复 |
