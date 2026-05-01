# 架构缺陷与技术债务清单

> 最后更新：2026-05-01

---

## Bug（需修复）

| 编号 | 描述 | 严重性 | 文件 | 状态 |
|------|------|--------|------|------|
| BUG-03 | `languageMap()` 返回引用但 mutex 已释放（数据竞争） | **Critical** | `GameInferService.cpp:65-68` | ✅ 已修复 |
| BUG-04 | MainWidget 析构时未等待 QtConcurrent 后台任务（use-after-free） | **High** | `GameInfer/gui/MainWidget.cpp:415-479` | ✅ 已修复 |
| BUG-05 | AsyncTask 同时继承 QObject 和 QRunnable(autoDelete)，若有 parent 则 double-free | **High** | `dsfw/AsyncTask.h:19` | ✅ 已修复 |
| BUG-06 | `AlignmentDecoder` 用 `.at()` 查 vocab_，未知音素直接 crash | Medium | `hubert-infer/AlignmentDecoder.cpp:160` | ✅ 已修复 |
| BUG-07 | 推理库内部 throw std::runtime_error 穿透 bool 返回值 API | Medium | `GameModel.cpp`, `HfaModel.cpp` | ✅ 已修复 |
| BUG-08 | `_WIN_X86` 宏从未在 CMake 中定义，条件分支为死代码 | Medium | `HfaModel.h:34`, `RmvpeModel.h:35` | ✅ 已修复 |
| BUG-09 | GameModel / RmvpeModel 公共头缺少 DLL 导出宏 | Medium | `GameModel.h`, `RmvpeModel.h` | ✅ 已修复 |

### BUG-03: languageMap() 数据竞争

```cpp
const std::map<std::string, int> &GameInferService::languageMap() const {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    return m_game->get_language_map(); // mutex 释放后调用方持有悬空引用
}
```

`lock_guard` 在函数返回时析构，调用方拿到的引用指向可被其他线程修改的内部数据。

**修复方向**: 返回 `std::map<std::string, int>`（按值拷贝），在 mutex 保护下完成拷贝。

### BUG-04: MainWidget 后台任务生命周期

`MainWidget::exportMidi()` 用 `QtConcurrent::run` 捕获裸 `this`。关闭窗口时未调用 `m_runningTask.waitForFinished()`，导致 lambda 中 `QMetaObject::invokeMethod(this, ...)` 访问已析构对象。

**修复方向**: 析构函数中 `m_runningTask.waitForFinished()`，或改用 `QPointer<MainWidget>` 捕获。

### BUG-05: AsyncTask double-free 风险

`AsyncTask` 同时继承 `QObject`（可有 parent 管理生命周期）和 `QRunnable`（默认 `autoDelete()=true`，QThreadPool 自动 delete）。两者同时生效时 double-free。

**修复方向**: 在 `AsyncTask` 构造函数中 `setAutoDelete(false)`，生命周期统一由 QObject parent 或手动管理。

### BUG-06: AlignmentDecoder 未知音素 crash

`vocab_.at(ph)` 在音素不在词表时抛 `std::out_of_range`。用户 TextGrid 中的拼写错误直接导致 crash。

**修复方向**: 改用 `vocab_.find(ph)`，未知音素跳过或报错返回。

### BUG-07: throw 穿透 bool API

`GameModel::forward()` 返回 `bool` + `std::string &errorMsg`，但内部调用的 `DiffSingerParser`、`NoteAlignment` 等 throw `std::runtime_error`。调用方不期望异常。`HfaModel` 同理。

**修复方向**: 在 `forward()` / `recognize()` 入口用 `try/catch(std::exception &e)` 包裹，将异常转为 errorMsg + return false。

### BUG-08: `_WIN_X86` 宏从未定义

`HfaModel.h`、`RmvpeModel.h`、`FunAsr/paraformer_onnx.h` 中有 `#ifdef _WIN_X86` 条件编译，但该宏从未在任何 CMakeLists.txt 中定义。`OrtArenaAllocator` 相关分支永远不会执行。

**修复方向**: 改用 MSVC 内建宏 `_M_IX86`，或在 CMake 中为 x86 平台 `target_compile_definitions`。

### BUG-09: GameModel / RmvpeModel 缺少导出宏

`GameModel` 和 `RmvpeModel` 是 SHARED 库的公共类，但头文件中没有 `GAME_INFER_EXPORT` / `RMVPE_INFER_EXPORT` 宏。MSVC 下依赖这些头文件的外部 target 可能链接失败。目前工作纯属巧合（直接链接 DLL）。

**修复方向**: 给两个类声明添加对应导出宏。

---

## 遗留缺陷

| 编号 | 描述 | 严重性 | 状态 |
|------|------|--------|------|
| AD-05 | dsfw-core 依赖 Qt::Gui (QColor 等) | Low | 延后（除非需要无头 CLI） |
| AD-06 | pipeline-pages 使用 `../slicer/` 相对路径源文件引用 | Low | 待做 |

### AD-05: core 依赖 Qt::Gui

dsfw-core 链接 `Qt::Gui`，纯 CLI 场景下仍需 Qt GUI 模块运行时。实际影响小（Qt::Gui 远轻于 Qt::Widgets）。

**修复方向**: 审查 QColor/QImage 使用点，替换为自定义轻量类型或仅在 widget 层使用。

### AD-06: pipeline-pages 相对路径源文件引用

pipeline-pages CMakeLists 通过 `../slicer/` 等相对路径引用源码，绕过了 CMake 目标依赖机制。

**修复方向**: 将 slicer/lyricfa/hubertfa 提取为独立静态库，通过 `target_link_libraries` 链接。

---

## 架构债

| 编号 | 描述 | 严重性 | 状态 |
|------|------|--------|------|
| ARCH-07 | labeler-interfaces 与 framework 接口重复（IID 版本不一致，ODR 风险） | **High** | ✅ 已修复 |
| ARCH-08 | 应用层直接 #include 推理库头文件，UI 中管理推理生命周期（层违反） | High | 待修 |
| ARCH-09 | 错误处理三套模式共存（Result\<T\> / bool+error / throw），推理层 throw 穿透 bool API | High | 待修 |
| ARCH-10 | MinLabelPage 大量业务逻辑（文件 I/O、格式转换）在 UI 层 | Medium | 待修 |
| ARCH-11 | 两套独立 Slicer 实现（audio-util vs libs/slicer），算法相同 | Medium | 待修 |
| ARCH-12 | IDocument.h 中 `registerDocumentFormat()` 使用文件作用域静态变量，线程不安全 | Medium | 待修 |
| ARCH-13 | dstools-widgets CMakeLists 冗余链接 dsfw-core（已通过 dstools-ui-core 传递） | Low | 待修 |

### ARCH-07: labeler-interfaces 接口重复

`src/apps/labeler-interfaces/include/` 中有 `IPageActions.h`、`IPageLifecycle.h`、`IPageProgress.h` 三个文件，与 `src/framework/ui-core/include/dsfw/` 中的同名文件内容近似但 **IID 版本字符串不一致**（labeler 用 `".../1.0"`，framework 无版本号）。`Q_DECLARE_INTERFACE` 的 IID 不同导致 `qobject_cast` 可能静默失败，属 ODR 违反风险。

**修复方向**: 删除 `src/apps/labeler-interfaces/`，所有消费者改用 `dsfw-ui-core` 提供的接口。

### ARCH-08: 应用层直接使用推理库

以下 UI 页面直接 `#include` 推理库头文件并管理引擎生命周期：
- `GameAlignPage.cpp` → `game-infer/Game.h`
- `MainWidget.h` (GameInfer) → `game-infer/Game.h`
- `HubertFAPage.cpp` → `hubert-infer/Hfa.h`

**修复方向**: 通过 domain 层服务接口（T-1.5 ISlicerService 等）间接使用推理能力。

### ARCH-09: 错误处理不一致

| 模式 | 使用位置 |
|------|---------|
| `Result<T>` | IFileIOProvider, IModelProvider, DsDocument, 服务接口 |
| `bool` + `std::string &error` | IDocument::load/save, IExportFormat, GameModel::load_model, OnnxEnv::createSession |
| `throw std::runtime_error` | DiffSingerParser（10 处）, GameModel（8 处）, AlignWord（3 处）, HfaModel, DictionaryG2P |

关键问题：`GameModel::forward()` 和 `HfaModel::forward()` 声明 `bool` 返回值，但内部未 catch 的 throw 直接穿透到调用方。

**修复方向**: 短期在 forward()/recognize() 入口 try/catch；长期统一为 `Result<T>`。

### ARCH-10: MinLabelPage 业务逻辑泄漏到 UI

`MinLabelPage.cpp`（~560 行）包含：直接读写 JSON 文件（QFile + QJsonDocument + nlohmann::json）、写 .lab 文件（QTextStream）、目录扫描（QDir::entryInfoList）、格式转换逻辑。

**修复方向**: 提取 `MinLabelService` 到 domain 层，页面仅调用服务接口。

### ARCH-11: Slicer 双实现

- `src/infer/audio-util/include/audio-util/Slicer.h` — C 风格，7 参数，操作原始样本数组
- `src/libs/slicer/Slicer.h` — Qt 风格，6 参数，操作 `SndfileHandle`

两者实现相同的 RMS 切片算法，互不引用。

**修复方向**: 以 audio-util 版本为底层实现（无 Qt 依赖），libs/slicer 作为薄封装调用 audio-util。

### ARCH-12: registerDocumentFormat() 静态变量

`IDocument.h` 中 `registerDocumentFormat()` 使用 `static int s_nextId` 和 `static std::unordered_map`，非线程安全且依赖初始化顺序。

**修复方向**: 迁移到 ServiceLocator 管理的注册表，或加 `std::mutex` 保护。

### ARCH-13: dstools-widgets 冗余依赖

`dstools-widgets` CMakeLists 中显式链接 `dsfw-core`，但已通过 `dstools-ui-core` PUBLIC 传递。不影响正确性但增加理解成本。

**修复方向**: 移除冗余的 `dsfw-core` 链接。

---

## 技术债

| 编号 | 描述 | 严重性 | 状态 |
|------|------|--------|------|
| TD-02 | 领域模块零测试覆盖 | 中 | 框架核心已有测试，领域类（DsDocument、CsvToDsConverter、PitchUtils 等）仍缺 |
| TD-03 | 文件操作缺少错误分支 | 低 | 部分修复 |
| TD-04 | God class：5 个文件超 500 行，职责过多 | 中 | 待修 |
| TD-05 | 魔法数字散布且重复（4096 buffer size 出现 3 处、窗口尺寸硬编码） | 低 | 待修 |
| TD-06 | 3 处 TODO/FIXME 标记未完成功能 | 低 | 待修 |

### TD-02: 领域模块零测试

**待测模块**: DsDocument, TranscriptionPipeline, CsvToDsConverter, TextGridToCsv, PitchUtils, F0Curve, PhNumCalculator, PitchProcessor

**已有测试**: TestResult, TestJsonHelper, TestAppSettings, TestServiceLocator, TestAsyncTask, TestLocalFileIOProvider, TestModelManager, TestModelDownloader, TestAppShellIntegration

### TD-03: 文件操作错误处理

多处 `file.open()` 无 else 分支。建议统一使用 `Result<T>` 包装文件操作返回值。

### TD-04: God class

| 文件 | 行数 | 问题 |
|------|------|------|
| `PitchLabelerPage.cpp` | 781 | 混合文件 I/O、UI 布局、音频播放、滚动条数学、undo 集成 |
| `PhonemeLabelerPage.cpp` | 630 | 混合文件加载、波形渲染、音频设置、滚动条管理 |
| `GameModel.cpp` | 602 | 5 个 ONNX session + 全部推理 pipeline 阶段在同一类 |
| `PianoRollView.cpp` | 579 | 渲染 + 输入处理 + 滚动缩放在同一 widget |
| `MainWidget.cpp` (GameInfer) | 505 | 完整 app UI + 推理编排在同一 widget |

**修复方向**: 逐步拆分——文件 I/O 提取到 service、滚动/缩放提取到 controller、渲染提取到 renderer。

### TD-05: 魔法数字

| 文件 | 值 | 含义 |
|------|------|------|
| `WaveformWidget.cpp:62`, `PhonemeLabelerPage.cpp:451`, `AudioFileLoader.cpp:60` | `4096` | 音频 buffer 大小（三处重复） |
| `SlicerPage.cpp:39` | `5000` | 默认最小切片长度 |
| `SpectrogramWidget.cpp:64` | `0.35875, 0.48829...` | Blackman-Harris 窗函数系数（无名常量） |
| `MinLabel/main.cpp:41` | `1280, 720` | 窗口尺寸 |
| `pipeline/main.cpp:44` | `1200, 800` | 窗口尺寸 |
| `BuildDsPage.cpp:36,42` | `64, 1024, 8000, 48000` | Hop size / 采样率范围 |

**修复方向**: 提取为命名常量。`4096` buffer size 统一到 `AudioConstants::kDefaultBufferSize`。

### TD-06: TODO/FIXME 标记

| 文件 | 内容 |
|------|------|
| `GameInferPage.cpp:60` | `// TODO: Forward file paths to MainWidget for processing` |
| `BuildDsPage.cpp:89` | `// TODO: Implement proper RMVPE-based F0 extraction when audio decode integration is ready` |
| `game-infer/tests/main.cpp:322` | `// TODO: Map language string to ID from config.json languages map` |
