# 编码规范违规审计报告

**审计日期**：2026-05-08
**验证日期**：2026-05-08（已对照源码逐项验证）
**审计范围**：src/ 全目录
**参考文档**：conventions-and-standards.md (P-01 ~ P-09, §1-§12)、human-decisions.md

---

## 汇总

| 规则 | 违规数 | 高 | 中 | 低 |
|------|--------|----|----|-----|
| P-01 模块职责单一 | 2 | 0 | 2 | 0 |
| P-02 被动接口+容器通知 | 5 | 2 | 3 | 0 |
| P-04 错误根因传播 | 3 | 2 | 1 | 0 |
| P-05 异常边界隔离 | 5 | 1 | 2 | 2 |
| P-09 异步任务存活令牌 | 7 | 5 | 1 | 0 |
| §12 路径 I/O | 7 | 2 | 3 | 2 |
| §11 头文件组织 | 3 | 0 | 0 | 3 |
| §4 日志规范 | 3 | 0 | 2 | 1 |
| §2 命名规范 | 2 | 0 | 2 | 0 |
| §9 CMake 规范 | 4 | 0 | 4 | 0 |
| QSettings 硬编码 | 3 | 0 | 2 | 1 |
| **合计** | **44** | **12** | **22** | **10** |

---

## 一、P-02 违规 — 被动接口+容器通知（数据接口继承 QObject）

### P02-01: IEditorDataSource 继承 QObject 并声明信号

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | IEditorDataSource.h:L26-28, L79-87 |

**描述**：`IEditorDataSource` 是纯数据接口却继承 QObject 并声明 3 个信号（sliceListChanged/sliceSaved/audioAvailabilityChanged），违反"纯数据接口不加 QObject"原则。所有实现类必须将 `QObject` 放在继承链首位，限制了实现灵活性，多重继承时容易产生菱形继承问题。

**修复方案**：将信号改为纯虚函数回调接口（如 `ISliceListObserver`），或者使用独立的信号提供者类，使接口不依赖 `QObject`。

**风险项**：
- 需要修改所有 `IEditorDataSource` 的实现类
- 需要重新设计信号通知机制（由容器负责）
- 工作量较大

---

### P02-02: IModelManager 继承 QObject 并声明信号

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | IModelManager.h:L20-21, L39-45 |

**描述**：`IModelManager` 是模型管理接口却继承 QObject 并声明 3 个信号（modelStatusChanged/memoryUsageChanged/modelInvalidated），通知应由持有它的容器分发。

**修复方案**：移除 QObject 基类和 Q_OBJECT 宏，信号改为容器级回调或 invalidateXxx()。`ModelManager` 实现类需要保留 QObject 特性（用于信号），但接口层应解耦。

**风险项**：
- `modelInvalidated` 信号被多个 Page 依赖，需要重新设计通知机制
- ModelManager 实现类仍需 QObject，但接口层应解耦

---

### P02-03: IAudioPlayer 继承 QObject 并声明信号

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | IAudioPlayer.h:L12-13, L71-77 |

**描述**：`IAudioPlayer` 是播放器接口却继承 QObject 并声明 2 个信号（stateChanged/deviceChanged），通知应由 AudioPlaybackManager 容器分发。

**修复方案**：移除 QObject 基类，信号由 AudioPlaybackManager 统一管理。

**风险项**：需要修改 `AudioPlayer` 实现类和所有消费者。

---

### P02-04: ISettingsBackend 继承 QObject 并声明信号

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | ISettingsBackend.h:L8-9, L17-18 |

**描述**：`ISettingsBackend` 是设置后端接口却继承 QObject 并声明 1 个信号（dataChanged），通知应由 AppSettings 容器分发。

**修复方案**：移除 QObject 基类和 Q_OBJECT 宏，dataChanged 由 AppSettings 统一通知。

**风险项**：低风险修复。

---

### P02-05: TextGridDocument 同时继承 QObject 和 IBoundaryModel

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | TextGridDocument.h:L17-18 |

**描述**：`TextGridDocument` 同时继承了 `QObject`（第一个基类）和 `IBoundaryModel`（纯接口，非 QObject）。`QObject` 必须是第一个基类，否则 `Q_OBJECT` 宏不工作。如果其他代码持有 `IBoundaryModel*` 指针，需要 `dynamic_cast` 才能转回 `QObject*`。

**修复方案**：信号通知应由 AudioVisualizerContainer 容器的 invalidateBoundaryModel() 统一分发。TextGridDocument 通过组合而非继承来提供 IBoundaryModel 功能。

**风险项**：TextGridDocument 的信号被多个组件依赖，需要逐步迁移。

---

## 二、P-04 违规 — 错误根因传播

### P04-01: MinLabelPage ASR 失败时错误消息缺少根因

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | MinLabelPage.cpp:L451 |

**描述**：`MinLabelPage::runAsrForSlice` 中 ASR 失败时仅记录 `"ASR failed: " + sliceId`，缺少根因（asr->recognize 的错误信息 msg 被忽略）。

**修复方案**：改为 `DSFW_LOG_ERROR("infer", ("ASR failed: " + sliceId.toStdString() + " - " + msg).c_str())`。

**风险项**：低风险修复。

---

### P04-02: libs/slicer/SlicerService::slice 使用 toLocal8Bit 打开音频文件

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | SlicerService.cpp:L14-16 |

**描述**：`libs/slicer/SlicerService::slice` 使用 `audioPath.toLocal8Bit()` 打开音频文件，在 Windows 上 CJK 路径会损坏，但错误消息仅说 "Failed to open audio file" 未指出编码根因。

**修复方案**：改用 `AudioUtil::openSndfile(toFsPath(audioPath))` 并在错误中包含路径编码信息。

**风险项**：需要确认 `AudioUtil::openSndfile` 和 `toFsPath` 在 libs/slicer 中的可用性。

---

### P04-03: SlicerService 错误消息使用 toStdString 丢失 CJK 字符

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | SlicerService.cpp:L11 |

**描述**：`libs/slicer/SlicerService::slice` 错误消息使用 `audioPath.toStdString()` 在 Windows 上会丢失 CJK 字符，导致错误信息本身不可读。

**修复方案**：使用 `audioPath.toUtf8().toStdString()` 或 `QString` 拼接。

**风险项**：低风险修复。

---

## 三、P-05 违规 — 异常边界隔离（业务逻辑中使用 try-catch）

### P05-01: TextGridDocument::save 中 try-catch 包裹非第三方代码操作

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | TextGridDocument.cpp:L101-123 |

**描述**：`TextGridDocument::save` 中 try-catch 同时包裹了 `textgrid` 库的序列化操作（第三方代码，可能抛异常）和 Qt 的文件操作（不抛异常）。根据设计原则，try-catch 应仅用于第三方库边界，Qt 文件操作不应被包含在内。

**修复方案**：将 try-catch 缩小到仅包裹 `textgrid` 库调用（`oss << m_textGrid`），Qt 文件操作移到 try-catch 外部：

```cpp
std::string content;
try {
    std::ostringstream oss;
    oss << m_textGrid;
    content = oss.str();
} catch (const std::exception &e) {
    qWarning() << "Failed to serialize TextGrid:" << e.what();
    return false;
}
QFile file(path);
// ...
```

**风险项**：低风险修复，仅缩小 try-catch 范围。

---

### P05-02: MatchLyric::loadLyricDict 中 try-catch 包裹内部库调用

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | MatchLyric.cpp:L26-31 |

**描述**：`MatchLyric::initLyric` 中 try-catch 包裹 `m_matcher->process_lyric_file()`，LyricFA 是内部库不是第三方库。内部库不应通过异常传递错误，应使用 `Result<T>` 或错误码。

**修复方案**：修改 `LyricMatcher::process_lyric_file` 返回 `Result<LyricData>` 而非抛异常，调用方用 `Result` 处理错误。

**风险项**：需要修改 LyricFA 的接口。

---

### P05-03: SettingsPage 中 try-catch 包裹 DictionaryG2P 构造函数

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | SettingsPage.cpp:L733-742 |

**描述**：`SettingsPage` 的 G2P 测试按钮回调中 try-catch 包裹 `HFA::DictionaryG2P` 构造函数，HFA 是内部库不是第三方库。

**修复方案**：修改 `DictionaryG2P` 的构造函数和 `convert` 方法返回 `Result<T>`，调用方用 `Result` 处理错误。

**风险项**：需要修改 DictionaryG2P 的接口。

---

### P05-04: hubert-infer/JsonUtils::loadFile 中 catch 范围过宽

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | JsonUtils.h:L65-68 |

**描述**：`catch (const std::exception &e)` 捕获范围过宽，可能吞掉非 nlohmann::json 异常。

**修复方案**：缩小 catch 范围为仅捕获 `nlohmann::json::exception`，或改为返回 Result<T>。

**风险项**：低风险修复。

---

### P05-05: JsonUtils::getRequiredMap/getRequiredVec 使用 out-parameter + bool

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | JsonUtils.h:L101-107, L129-135 |

**描述**：try-catch 包裹 `node->get<T>()`，虽然是 nlohmann::json 方法但应转为 Result 而非返回空对象+error out-param。

**修复方案**：改为返回 `Result<T>` 而非 out-parameter + bool。

**风险项**：需要修改所有调用点。

---

## 四、P-09 违规 — 异步任务存活令牌

### P09-01 ~ P09-03: PitchLabelerPage 三处 QtConcurrent::run 缺少引擎存活令牌

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | PitchLabelerPage.cpp:L514 (rmvpe), L566 (game), L824 (batch rmvpe+game) |

**描述**：`PitchLabelerPage` 在 `QtConcurrent::run` 中捕获 `rmvpe` 和 `game` 原始指针，缺少 `std::shared_ptr<std::atomic<bool>>` 引擎存活令牌。对比 `PhonemeLabelerPage` 已正确实现 `m_hfaAlive`（L446 初始化，L553-554 检查，L251 store(false) + cancelAsyncTask）。

**修复方案**：添加 `m_rmvpeAlive` 和 `m_gameAlive` 成员，lambda 中捕获并检查 `if (!rmvpeAlive || !*rmvpeAlive) return`。

**风险项**：修复后仍存在 TOCTOU 间隙，但已大幅降低崩溃概率。

---

### P09-04 ~ P09-05: MinLabelPage 两处 QtConcurrent::run 缺少引擎存活令牌

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | MinLabelPage.cpp:L418 (asr), L495 (batch asr) |

**描述**：同 P09-01，`MinLabelPage` 缺少 `m_asrAlive` 存活令牌。

**修复方案**：添加 `m_asrAlive` 成员。

**风险项**：同 P09-01。

---

### P09-06: MinLabelPage::onBatchLyricFa 缺少存活令牌

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | MinLabelPage.cpp:L728 |

**描述**：`QtConcurrent::run` 中捕获 `matchLyric` 原始指针，缺少存活令牌。

**修复方案**：添加 `m_matchLyricAlive` 成员。

**风险项**：低风险修复。

---

### P09-07: ExportPage::continueExport 缺少引擎存活令牌

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | ExportPage.cpp:L596 |

**描述**：`QtConcurrent::run` 中捕获 `hfa/rmvpe/game/phNumCalc` 原始指针，缺少引擎存活令牌。

**修复方案**：为每个引擎添加存活令牌。

**风险项**：同 P09-01。

---

## 五、路径 I/O 违规 (§12) — path.string() 用于文件 I/O

### PATH-01: Hfa.cpp 中 DictionaryG2P 使用 path.string() 做文件 I/O

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | Hfa.cpp:L62, DictionaryG2P.cpp:L9/L11 |

**描述**：`DictionaryG2P(dict_path.string(), language)` 使用 path.string() 传递路径给 DictionaryG2P 构造函数做文件 I/O，Windows 上 CJK 路径会损坏。DictionaryG2P 构造函数接收 `const std::string &dictionaryPath`，内部 `std::ifstream file(dictionaryPath)` 打开文件。

**修复方案**：改为 `DictionaryG2P(dict_path, language)`，DictionaryG2P 构造函数参数改为 `std::filesystem::path`。

**风险项**：需要修改 DictionaryG2P 接口。

---

### PATH-02: libs/slicer/SlicerService::slice 使用 toLocal8Bit 打开音频

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | SlicerService.cpp:L14 |

**描述**：`SndfileHandle(audioPath.toLocal8Bit().constData())` 打开音频，Windows 上 CJK 路径损坏。应使用 `AudioUtil::openSndfile`。项目已有正确的跨平台实现 `PathCompat::openSndfile()`（Windows 上使用 `path.wstring().c_str()`）。

**修复方案**：改为 `AudioUtil::openSndfile(dstools::toFsPath(audioPath))`。

**风险项**：需要确认 `AudioUtil::openSndfile` 在 libs/slicer 中的可用性。

---

### PATH-03: Hfa.cpp 使用 path.string() 输出到 stderr

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | Hfa.cpp:L60/L89/L119/L127/L197 |

**描述**：多处 `dict_path.string()` / `model_folder.string()` / `wavPath.string()` / `labPath.string()` 输出到 std::cerr 或错误消息，Windows 上 CJK 路径显示乱码。

**修复方案**：使用 `dict_path.wstring()` 或 `PathUtils::fromStdPath(dict_path)`。

**风险项**：低风险修复。

---

### PATH-04: JsonHelper.cpp 多处使用 path.string() 拼接错误消息

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | JsonHelper.cpp:L10, L25, L29 |

**描述**：`path.string()` 拼接错误消息，Windows 上 CJK 路径显示乱码。注意：`std::ifstream file(path)` 和 `std::ofstream file(path)` 在 MSVC 上接受 `std::filesystem::path`，文件打开本身没问题。

**修复方案**：使用 `PathUtils::fromStdPath(path).toStdString()` 或 `path.u8string()`。

**风险项**：低风险修复。

---

### PATH-05: hubert-infer/JsonUtils.h 多处使用 path.string() 拼接错误消息

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | JsonUtils.h:L47, L52, L58, L63, L66 |

**描述**：同 PATH-04。

**修复方案**：同 PATH-04。

**风险项**：低风险修复。

---

### PATH-06: GameModel.cpp 使用 path.string() 拼接错误消息

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | GameModel.cpp:L40, L84 |

**描述**：同 PATH-04。注意：L88-91 ONNX Session 创建已正确使用 `model_path.wstring().c_str()` (Windows)。

**修复方案**：使用 `path.u8string()` 或平台相关转换。

**风险项**：低风险修复。

---

### PATH-07: audio-util/Util.cpp 使用 filepath.string() 拼接错误消息

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | Util.cpp:L75 |

**描述**：同 PATH-04。

**修复方案**：同 PATH-06。

**风险项**：低风险修复。

---

## 六、头文件组织违规 (§11) — 缺少 #pragma once

### HDR-01: FunAsr 内部头文件使用 #ifndef 守卫

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | util.h, Tensor.h, Vocab.h, SpeechWrap.h, predefine_coe.h, FeatureQueue.h, FeatureExtract.h, alignedmem.h |

**描述**：FunAsr 内部头文件使用 `#ifndef` 宏保护而非 `#pragma once`，不符合项目规范 §11。

**修复方案**：统一替换为 `#pragma once`。

**风险项**：FunAsr 是第三方代码，修改需谨慎评估。

---

### HDR-02: FunAsr 公开头文件使用 #ifndef 守卫

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | Model.h, ComDefine.h, Audio.h |

**描述**：同 HDR-01。

**修复方案**：同 HDR-01。

**风险项**：同 HDR-01。

---

### HDR-03: paraformer_onnx.h 同时使用 #ifndef 和 #pragma once

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | paraformer_onnx.h |

**描述**：同时使用两种头文件守卫，冗余。

**修复方案**：移除 `#ifndef` 守卫，仅保留 `#pragma once`。

**风险项**：极低风险。

---

## 七、日志规范违规 (§4)

### LOG-01: CrashHandler.cpp 使用非规范日志分类

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | CrashHandler.cpp:L45, L109, L118 |

**描述**：使用 `"CrashHandler"` 作为日志分类，不在规范定义的分类列表中（应使用 `"app"`）。

**修复方案**：改为 `DSFW_LOG_ERROR("app", ...)` / `DSFW_LOG_INFO("app", ...)`。

**风险项**：低风险修复。

---

### LOG-02: TextGridDocument.cpp 使用非规范日志分类

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | TextGridDocument.cpp:L404 |

**描述**：使用 `"data"` 作为日志分类，不在规范定义的分类列表中（应使用 `"io"`）。

**修复方案**：改为 `DSFW_LOG_WARN("io", ...)`。

**风险项**：低风险修复。

---

### LOG-03: 多处使用 qWarning()/qDebug()/qCritical() 而非 DSFW_LOG

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认（15+ 处） |
| **影响文件** | TextGridDocument.cpp (7处), AudioDecoder.cpp (6处), AudioPlayback.cpp (2处), AppSettings.cpp (4处), Theme.cpp (1处), FramelessHelper.cpp (2处), PlayWidget.cpp (1处), WaveformWidget.cpp (1处), DsProject.cpp (2处) |

**描述**：多处使用 `qWarning()`/`qDebug()`/`qCritical()` 而非 `DSFW_LOG_WARN/DEBUG/ERROR`，绕过统一日志系统。这意味着这些日志不会写入 FileLogSink，无法在生产环境中追溯。

**修复方案**：统一替换为 `DSFW_LOG_WARN/DEBUG/ERROR` 宏，使用正确的分类字符串：
- AudioDecoder/AudioPlayback → `"audio"`
- AppSettings → `"settings"`
- TextGridDocument → `"io"`
- Theme/FramelessHelper → `"ui"`
- DsProject → `"io"`

**风险项**：
- 工作量较大（涉及多个文件）
- 需要为每条日志选择正确的分类字符串

---

## 八、命名规范违规 (§2) — include 前缀

### NAME-01: framework/audio 使用 dstools/ 前缀而非 dsfw/

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | framework/audio/include/dstools/ 下 5 个头文件：AudioDecoder.h, AudioPlayback.h, AudioPlayer.h, IAudioPlayer.h, WaveFormat.h |

**描述**：`src/framework/audio/` 是框架模块，但使用 `<dstools/...>` include 前缀，应使用 `<dsfw/...>`。根据 framework-architecture.md §10 命名规范表，框架层应使用 `<dsfw/...>` 前缀。对比 `dsfw-core` 模块正确使用了 `<dsfw/Log.h>` 前缀。

**修复方案**：将 include 目录从 `dstools/` 改为 `dsfw/`，更新所有 `#include <dstools/AudioDecoder.h>` 等引用。

**风险项**：
- 需要修改 CMakeLists.txt 中的 include 目录配置
- 需要更新所有引用
- 可能影响下游消费者

---

### NAME-02: framework/infer 使用 dstools/ 前缀而非 dsfw/

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | framework/infer/include/dstools/ 下 3 个头文件：IInferenceEngine.h, OnnxEnv.h, OnnxModelBase.h |

**描述**：同 NAME-01。

**修复方案**：同 NAME-01。

**风险项**：同 NAME-01。

---

## 九、QSettings 硬编码 key 违规

### QSET-01: WelcomePage 直接使用 QSettings 并硬编码 key

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | WelcomePage.cpp:L22, L89-92, L172-178, L184-185 |

**描述**：直接使用 QSettings 并硬编码 key `"App/recentProjects"`，未使用 `SettingsKey` 常量或 `AppSettings`。

**修复方案**：在 `DsLabelerKeys.h` 中定义 `SettingsKey<QStringList>` 常量，改用 `AppSettings`。

**风险项**：低风险修复。

---

### QSET-02: SettingsPage 直接使用 QSettings 并硬编码 key

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | SettingsPage.cpp:L461, L480 |

**描述**：直接使用 QSettings 并硬编码 key `"App/language"`，未使用 `CommonKeys::Language`。

**修复方案**：改用 `AppSettings` + `CommonKeys::Language`。

**风险项**：低风险修复。

---

### QSET-03: AppSettingsBackend 大量硬编码 QSettings key

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | AppSettingsBackend.cpp:L13-76 |

**描述**：大量硬编码 QSettings key（如 `"Settings/globalProvider"`, `"Settings/deviceIndex"` 等），应定义为 SettingsKey 常量。

**修复方案**：将 QSettings key 提取为 `SettingsKey` 常量，统一管理。

**风险项**：低风险修复，但工作量较大。

---

## 十、CMake 依赖可见性违规 (§9) — PUBLIC vs PRIVATE

### CMAKE-01: hubert-infer 将 dstools-infer-common 声明为 PUBLIC

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | hubert-infer/CMakeLists.txt:L17-18 |

**描述**：`hubert-infer` 将 `dstools-infer-common` 声明为 PUBLIC 依赖，SndFile/nlohmann_json/audio-util 为 PRIVATE。需检查公共头文件是否实际暴露了 dstools-infer-common 的类型。

**修复方案**：检查公共头文件是否实际暴露了 dstools-infer-common 的类型，若否则改为 PRIVATE。

**风险项**：改为 PRIVATE 后下游消费者可能缺少必要的 include 路径。

---

### CMAKE-02: rmvpe-infer 将 SndFile/audio-util 声明为 PUBLIC

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | rmvpe-infer/CMakeLists.txt:L17 |

**描述**：`rmvpe-infer` 将 `SndFile::sndfile` 和 `audio-util` 声明为 PUBLIC 依赖。这些底层音频 I/O 库不应在公开头文件中暴露。

**修复方案**：将 `SndFile::sndfile` 和 `audio-util` 改为 PRIVATE。

**风险项**：同 CMAKE-01。

---

### CMAKE-03: game-infer 将所有依赖声明为 PUBLIC

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | game-infer/CMakeLists.txt:L19 |

**描述**：`game-infer` 将 `SndFile::sndfile`、`nlohmann_json::nlohmann_json`、`audio-util`、`wolf-midi::wolf-midi` 全部声明为 PUBLIC。这些底层实现库几乎不可能在公开头文件中使用。

**修复方案**：将 `SndFile::sndfile`、`audio-util`、`wolf-midi::wolf-midi` 改为 PRIVATE。`nlohmann_json` 需检查公共头文件是否使用了 `nlohmann::json` 类型。

**风险项**：同 CMAKE-01。

---

### CMAKE-04: lyricfa-lib 将 SndFile/nlohmann_json 声明为 PUBLIC

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | lyric-fa/CMakeLists.txt:L11-14 |

**描述**：`lyricfa-lib` 将 `SndFile::sndfile`、`audio-util`、`Qt::Widgets` 声明为 PUBLIC。这些底层库不应在公开头文件中暴露。

**修复方案**：检查公共头文件，将仅实现内部使用的依赖改为 PRIVATE。`Qt::Widgets` 如果公共头文件不使用 Widgets 类型也应改为 PRIVATE。

**风险项**：同 CMAKE-01。

---

## 十一、P-01 违规 — 模块职责单一（行为分散重复）

### P01-01: 存在两个 SlicerService — 行为分散

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | apps/shared/data-sources/SlicerService.h (dstools 命名空间, 静态工具类), libs/slicer/SlicerService.h (全局命名空间, ISlicerService 实现) |

**描述**：两个 `SlicerService` 名称相同但职责不同：
- `libs/slicer/SlicerService` 是 `ISlicerService` 接口的实现，用于通过 `ServiceLocator` 提供切片服务
- `apps/shared/data-sources/SlicerService` 是应用层的静态工具类，提供更底层的切片操作和导出功能

这会导致命名混淆，且 apps 中的版本没有遵循接口模式。

**修复方案**：将 `apps/shared/data-sources/SlicerService` 重命名为 `SliceExportService` 或 `SlicerUtil`，以区分职责并避免命名冲突。

**风险项**：需要确认两个 SlicerService 的功能差异，避免重命名后遗漏功能。

---

### P01-02: 混音逻辑在两处重复实现

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | libs/slicer/SlicerService.cpp:L37-51 vs apps/shared/data-sources/SlicerService.cpp:L35-70 |

**描述**：`libs/slicer/SlicerService::slice` 和 `apps/shared/data-sources/SlicerService::loadAndMixAudio` 都实现了"读取音频→混音为单声道"的逻辑，行为重复。

**修复方案**：将混音逻辑提取到 `AudioUtil` 或 `audio::AudioDecoder` 中统一实现。

**风险项**：需要确认两处混音逻辑的参数差异。

---

## 优先修复建议

### 最高优先级（高严重度，安全/数据风险）

| ID | 描述 | 工作量 |
|----|------|--------|
| P09-01~07 | 7 处 QtConcurrent::run 缺少引擎存活令牌 | 中 |
| PATH-01~02 | 2 处路径 I/O 在 Windows 上损坏 CJK 字符 | 中 |
| P04-01~02 | 2 处错误缺少根因传播 | 小 |
| P02-01~02 | 2 处核心数据接口违反被动接口原则 | 大 |

### 中等优先级（中严重度，架构/规范问题）

| ID | 描述 | 工作量 |
|----|------|--------|
| P05-01~03 | 3 处业务逻辑中的 try-catch | 中 |
| NAME-01~02 | 框架模块使用错误的 include 前缀 | 中 |
| LOG-01~03 | 日志分类和 API 使用不规范 | 中 |
| CMAKE-01~04 | 依赖可见性声明不当 | 小 |
| QSET-01~02 | QSettings 硬编码 key | 小 |

### 低优先级（低严重度，风格/一致性问题）

| ID | 描述 | 工作量 |
|----|------|--------|
| HDR-01~03 | FunAsr 头文件守卫风格不统一 | 小 |
| PATH-06~07 | 错误消息中 path.string() 显示问题 | 小 |
| P05-04~05 | JsonUtils 异常处理改进 | 小 |
