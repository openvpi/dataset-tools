# 技术债审计报告

**审计日期**：2026-05-08
**验证日期**：2026-05-08（已对照源码逐项验证）
**审计范围**：src/apps/、src/framework/、src/domain/、src/libs/
**参考文档**：conventions-and-standards.md、human-decisions.md

---

## 汇总

| 严重度 | 数量 |
|--------|------|
| 高 | 5 |
| 中 | 9 |
| 低 | 4 |
| **合计** | **18** |

---

## TD-01: QMetaMethod 运行时反射 — 静默失败风险

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认（4 处） |
| **影响文件** | AudioVisualizerContainer.cpp:L365-374, L527-535, L547-559, L650-658 |

**描述**：`AudioVisualizerContainer` 在 4 处使用 `QMetaMethod` 按名称反射调用 `setViewport`、`setPlayhead`、`setDragController` 方法。

**验证补充**：
1. `addChart()` L365-374 — 反射调用 `setDragController`：遍历 `metaObject()->methodCount()`，按名称匹配 `"setDragController"`
2. `setPlayWidget()` L527-535 — 反射调用 `setPlayhead`：使用 `QGenericArgument("double", &secCopy)` 传递参数，类型不安全
3. `setPlayWidget()` L547-559 — 同上，清除播放头
4. `connectViewportToWidget()` L650-658 — 反射调用 `setViewport`：使用 `method.parameterTypes().at(0).constData()` 传递参数，若参数类型不匹配会崩溃

**修复方案**：定义 `IChartWidget` 接口（纯虚类），包含 `setViewport()`、`setPlayhead()`、`setDragController()` 等虚方法。所有 chart widget 实现此接口，`addChart()` 接受 `IChartWidget*` 并直接调用虚方法。这消除了反射开销和静默失败风险，也符合 P-02（被动数据接口）原则。

**风险项**：
- 需要修改所有 chart widget 的继承关系
- 需要确保接口方法签名与当前反射调用一致
- 修改期间可能影响视口同步功能

---

## TD-02: PitchLabelerPage / MinLabelPage / ExportPage 原始指针缺少存活令牌

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | PitchLabelerPage.cpp:L514/L566/L824, MinLabelPage.cpp:L418/L495/L728, ExportPage.cpp:L596 |

**描述**：根据 P-09 决策，所有 `QtConcurrent::run` 中捕获的原始指针必须配合 `std::shared_ptr<std::atomic<bool>>` 存活令牌使用。目前只有 `PhonemeLabelerPage` 正确实现了 `m_hfaAlive` 令牌模式（L446 初始化，L553-554 检查，L251 store(false) + cancelAsyncTask），其他三个页面均未实现。

**验证补充**：
- PitchLabelerPage.h：**没有** `m_rmvpeAlive` 或 `m_gameAlive` 成员，仅使用 `QPointer<PitchLabelerPage>` 作为 guard（只保护页面对象本身，不保护引擎指针有效性）
- MinLabelPage.h：**没有** `m_asrAlive` 成员
- ExportPage.h：**没有** alive token 成员

**修复方案**：为 PitchLabelerPage 添加 `m_rmvpeAlive` / `m_gameAlive`，为 MinLabelPage 添加 `m_asrAlive`，为 ExportPage 添加 `m_hfaAlive` / `m_rmvpeAlive` / `m_gameAlive`。在 `onDeactivatedImpl()` / `onDeactivated()` 中 store(false)，在 `QtConcurrent::run` lambda 入口处检查 alive token。参考 PhonemeLabelerPage 的实现模式。

**风险项**：
- 修复后仍存在 TOCTOU 间隙（见 TD-17），但已大幅降低崩溃概率
- 需要确保所有 `onModelInvalidated` 槽在引擎销毁前执行

---

## TD-03: AudioPlaybackManager 持有裸 QWidget 指针 — 潜在 use-after-free

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | AudioPlaybackManager.h:L22, AudioPlaybackManager.cpp:L22-23/L38-39 |

**描述**：`AudioPlaybackManager::m_currentPlayer` 是一个裸 `QWidget*`（L22），没有任何守卫。当 PlayWidget 被页面销毁（如页面切换时 `deleteLater()`），`m_currentPlayer` 变成悬空指针。后续 `requestPlay()` 或 `stopAll()` 调用 `QMetaObject::invokeMethod(m_currentPlayer, "setPlaying", ...)` 会访问已释放内存。

**修复方案**：将 `QWidget *m_currentPlayer` 改为 `QPointer<QWidget> m_currentPlayer`，在 `requestPlay()` 和 `stopAll()` 中检查 `if (!m_currentPlayer) return;`。同时考虑用接口替代 `QMetaObject::invokeMethod`（与 TD-13 联动）。

**风险项**：低风险修复，但需确认 `stopAll()` 的调用时机。

---

## TD-04: AppShell 使用 SIGNAL/SLOT 旧式连接 — 编译期无法检查

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | AppShell.cpp:135-136 |

**描述**：`AppShell::addPage()` 使用旧式 `SIGNAL()/SLOT()` 宏连接 `playRequested()` 和 `playStopped()` 信号。旧式连接在编译期无法检查信号/槽签名是否匹配。

**修复方案**：定义一个 `IPlayable` 接口（纯虚类，含 `playRequested()`/`playStopped()` 信号），让 PlayWidget 实现此接口。AppShell 通过 `qobject_cast<IPlayable*>` 获取接口指针后使用新式 `connect`。

**风险项**：需要解决 ui-core 和 widgets 模块间的循环依赖问题（这是使用旧式连接的原始原因）。

---

## TD-05: SliceBoundaryModel 无 QObject 父对象 — 内存泄漏

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | DsSlicerPage.cpp:58, SlicerPage.cpp:52 |

**描述**：`SliceBoundaryModel` 使用 `new` 创建但未传入 parent 对象，也没有被任何智能指针管理。

**修复方案**：将 `m_boundaryModel` 改为 `std::unique_ptr<phonemelabeler::SliceBoundaryModel>`，确保异常安全的资源管理。

**风险项**：低风险修复。

---

## TD-06: JsonHelper::get() 的 catch(...) 静默吞掉异常 — 违反 P-05

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | JsonHelper.h:30 |

**描述**：`JsonHelper::get<T>()` 模板方法中的 `catch(...)` 块静默返回 `defaultValue`，不记录日志也不报告错误。

**修复方案**：至少添加 `DSFW_LOG_WARN` 日志记录，或者改为返回 `Result<T>` 让调用者决定如何处理。

**风险项**：改为 `Result<T>` 会影响所有调用点；添加日志是低风险修复。

---

## TD-07: path.string() 在 Windows 上破坏 CJK 路径 — 违反 §12

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认（多处） |
| **影响文件** | PathCompat.h:L29, Hfa.cpp:L60/L62/L89/L119/L127/L197, DictionaryG2P.cpp:L9/L11, GameModel.cpp:L40/L84, SlicerService.cpp:L14, JsonHelper.cpp:L10/L25/L29, JsonUtils.h:L47/L52/L58/L63/L66, PathUtils.cpp:L38, Util.cpp:L75 |

**描述**：根据项目规范 §12，推理层代码中禁止使用 `path.string()` 进行文件 I/O（Windows 上会破坏 CJK 字符）。

**验证补充**（按严重程度排序）：
1. **Hfa.cpp:L62** — `DictionaryG2P(dict_path.string(), language)` 传给 DictionaryG2P 构造函数做文件 I/O，Windows 上 CJK 路径打开失败。DictionaryG2P 构造函数接收 `const std::string &dictionaryPath`（L9），内部 `std::ifstream file(dictionaryPath)`（L11）
2. **SlicerService.cpp:L14** — `SndfileHandle sf(audioPath.toLocal8Bit().constData())` 打开音频，Windows 上 CJK 路径损坏
3. **GameModel.cpp:L88-91** — ONNX Session 创建已正确使用 `model_path.wstring().c_str()` (Windows) / `model_path.c_str()` (非 Windows)，但错误消息（L40/L84）仍使用 `path.string()` 乱码
4. **JsonHelper.cpp** — `std::ifstream file(path)` 和 `std::ofstream file(path)` 在 MSVC 上接受 `std::filesystem::path`，文件打开本身没问题，只是错误消息乱码
5. **PathCompat.h** — 已有正确的跨平台实现 `openSndfile()`，Windows 上使用 `path.wstring().c_str()`

**修复方案**：
1. `DictionaryG2P` 构造函数参数改为 `std::filesystem::path`，内部用 `std::ifstream(path)` 打开。调用处改为 `DictionaryG2P(dict_path, language)`
2. `SlicerService.cpp` 使用 `AudioUtil::openSndfile()` 替代直接构造 `SndfileHandle`
3. 所有错误消息中使用 `PathUtils::fromStdPath()` 或 `path.u8string()` 转换

**风险项**：
- 需要区分 Windows 和 Unix 平台的路径转换逻辑
- 修改 `DictionaryG2P` 接口可能影响其他调用者
- 需要确认 `AudioUtil::openSndfile` 在 libs/slicer 中的可用性

---

## TD-08: SpectrogramWidget 性能问题 — setPixel 逐像素写入 + 全量重建

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | SpectrogramWidget.cpp:L289, L488-489 |

**描述**：`rebuildViewImage()` 存在两个严重性能问题：

1. **`QImage::setPixel()` 逐像素写入**（L289）：在双重循环中（L252 `for (int x = 0; x < w; ++x)` + L271 `for (int y = 0; y < h; ++y)`），`setPixel()` 是 Qt 中最慢的像素设置方式。对于 1000x400 的图像，需要调用 400,000 次
2. **`resizeEvent` 触发全量重建**（L488-489）：每次窗口大小变化都重新计算整个可见区域的像素
3. **缓存检查仅基于尺寸和视口范围**（L231-234），但每次滚动都会触发完整重建

**修复方案**：
1. 将 `setPixel()` 替换为 `QImage::scanLine()` 直接写入行缓冲区，性能可提升 10-50 倍
2. 在 `resizeEvent` 中使用 `QTimer::singleShot(0, ...)` 延迟重建，避免连续 resize 时的重复计算
3. 对于滚动操作，实现增量更新（只重绘新增区域，平移已有内容）

**风险项**：
- `scanLine()` 需要注意 QImage 格式（Format_RGB32 vs Format_ARGB32）
- 延迟重建可能导致短暂的白屏闪烁

---

## TD-09: PitchLabelerPage 硬编码中文字符串 — 缺少 tr() 包裹

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | PitchLabelerPage.cpp:L789/L792/L797/L802 |

**描述**：`PitchLabelerPage` 的批量提取对话框使用 `QStringLiteral()` 硬编码中文字符串，而非 `tr()` 包裹。

**修复方案**：将所有 `QStringLiteral("中文...")` 替换为 `tr("中文...")`，并更新 `.ts` 翻译文件。

**风险项**：低风险修复。

---

## TD-10: 废弃 API 仍在使用 — BatchCheckpoint / TranscriptionPipeline / DsItemManager

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | BatchCheckpoint.h:L11, TranscriptionPipeline.h:L17, DsItemManager.h:L16, DsDocument.h:L52/L62/L64 |

**描述**：项目中有多个已标记 `[[deprecated]]` 的类和方法仍然存在，测试中甚至需要用 `#pragma GCC diagnostic ignored "-Wdeprecated-declarations"` 来抑制警告。

**修复方案**：在下一个主版本中移除这些废弃 API，将测试迁移到新 API（`PipelineRunner`、`PipelineContext`）。

**风险项**：需要确认没有外部消费者依赖这些 API。

---

## TD-11: FunAsr 内部头文件使用 #ifndef 而非 #pragma once — 违反 §11

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | FunAsr/src/ 下 8 个头文件, FunAsr/include/ 下 3 个头文件, paraformer_onnx.h 同时使用两种守卫 |

**描述**：根据项目规范 §11，头文件保护统一使用 `#pragma once`。FunAsr 第三方库的所有内部头文件使用 `#ifndef` 宏保护，不符合项目规范。`paraformer_onnx.h` 同时使用了两种守卫，冗余。

**修复方案**：将 FunAsr 头文件的 `#ifndef/#define/#endif` 替换为 `#pragma once`。

**风险项**：FunAsr 是第三方代码，修改需谨慎评估。

---

## TD-12: SpectrogramWidget 硬编码魔法数字

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | SpectrogramWidget.h:L116/L123-125/L134/L139-141, SpectrogramWidget.cpp:L20-22 |

**描述**：`SpectrogramWidget` 中存在多个硬编码值：`m_sampleRate = 44100`、`m_viewEnd = 10.0`、`#define M_PI 3.14159265358979323846`（C++20 已有 `std::numbers::pi`）、`kMaxFrequency = 8000.0` 等。

**修复方案**：
1. 使用 `#include <numbers>` 和 `std::numbers::pi` 替代自定义 `M_PI`
2. 将 `kMaxFrequency`、`kMinIntensityDb`、`kMaxIntensityDb` 等参数暴露为可配置属性
3. 移除 `m_viewEnd = 10.0` 的默认值，改为从 ViewportController 初始化

**风险项**：低风险修复。

---

## TD-13: AudioPlaybackManager 使用 QMetaObject::invokeMethod 字符串调用 — 静默失败风险

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | AudioPlaybackManager.cpp:L22-23/L38-39 |

**描述**：`AudioPlaybackManager` 使用 `QMetaObject::invokeMethod(m_currentPlayer, "setPlaying", ...)` 通过字符串名称调用 `setPlaying` 方法。与 TD-03 联动——裸指针 + 字符串调用双重风险。

**修复方案**：定义一个 `IPlayable` 接口含 `setPlaying(bool)` 虚方法，或直接 `qobject_cast` 到具体类型后调用。与 TD-04 联动，建议一起修复。

**风险项**：需要解决 ui-core 和 widgets 模块间的循环依赖问题。

---

## TD-14: ExportPage 后台线程直接调用 source()->loadSlice() — 线程安全风险

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | ExportPage.cpp:L606, MinLabelPage.cpp:L742, PitchLabelerPage.cpp:L848 |

**描述**：多个页面的批量处理 `QtConcurrent::run` lambda 中调用了 `src->loadSlice(sliceId)`。`IEditorDataSource::loadSlice()` 没有任何线程安全文档或保证（IEditorDataSource.h:L38），其实现可能访问非线程安全的资源。

**验证补充**：
- ExportPage.cpp:L606 — `auto result = src->loadSlice(sliceId)` 在后台线程调用
- ExportPage.cpp:L837 — `guard->m_source->saveSlice(sliceId, doc)` 通过 `QMetaObject::invokeMethod` 回到主线程保存（正确做法），但 loadSlice 在后台线程（不一致）
- MinLabelPage.cpp:L742 — `auto result = src->loadSlice(sliceId)` 在后台线程调用

**修复方案**：
1. 在 `IEditorDataSource` 文档中明确标注 `loadSlice()` / `saveSlice()` 是否线程安全
2. 如果不线程安全，后台线程应通过 `QMetaObject::invokeMethod` 回到主线程执行 `loadSlice()`
3. 或者让 `ProjectDataSource` 内部加锁保证线程安全

**风险项**：
- 预加载方案可能增加内存占用
- mutex 方案可能降低并发性能
- 需要审查 `IEditorDataSource` 的所有实现

---

## TD-15: IntervalTierView TODO 注释 — 未完成功能

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | IntervalTierView.cpp:134 |

**描述**：`IntervalTierView::paintEvent()` 中有 `// TODO: draw cross-tier binding indicator lines during drag` 注释，与 D-27 决策中 `BoundaryDragController` 的 partners 可视化需求相关。

**修复方案**：在 `BoundaryDragController` 完成后（D-27），实现拖动时的跨层绑定指示线绘制。

**风险项**：依赖 D-27 的完成。

---

## TD-16: PianoRollView 仍使用已废弃的 setPixelsPerSecond API

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | PianoRollView.cpp:L86/L314/L342/L550/L674, ViewportController.h:L94 |

**描述**：根据 D-26 决策，`pixelsPerSecond` 概念和 API 应全部删除，替换为 `resolution`。ViewportController.h L7 注释明确说明 "The legacy 'pixelsPerSecond' concept is intentionally absent — see D-26"，但 L94 仍保留了 `setPixelsPerSecond()` 方法（标注为 "legacy compatibility"）。PianoRollView 是唯一仍在使用 PPS 的组件，内部维护了 `m_hScale`（pixels per second）作为主要缩放状态。

**修复方案**：
1. 将 PianoRollView 的 `m_hScale` 替换为基于 resolution 的计算
2. 使用 `m_viewport->setResolution()` 替代 `m_viewport->setPixelsPerSecond()`
3. 最终从 ViewportController 中移除 `setPixelsPerSecond()` 方法

**风险项**：需要确保 PitchEditor 的缩放行为与迁移前一致。

---

## TD-17: ModelManager::invalidateModel 的 TOCTOU 间隙 — 已知但未完全解决

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | ModelManager.cpp:L172-179, PhonemeLabelerPage.cpp:L554-558 |

**描述**：虽然 D-32 已将 `invalidateModel()` 改为先 emit 再 unload，且 P-09 引入了 alive token 模式，但 human-decisions.md 明确指出此方案存在 TOCTOU（check-then-use）间隙：alive token 检查和引擎使用之间可能被中断。

**修复方案**：彻底解决需要引擎层自身提供 `shared_ptr` 所有权或线程安全的访问令牌。作为过渡方案，可以在 `ModelManager::unload()` 中加 mutex，在引擎方法调用中也加同一 mutex，确保 check 和 use 之间不会被 unload 打断。

**风险项**：
- `shared_ptr` 方案需要修改所有推理引擎的生命周期管理
- mutex 方案可能影响推理性能（长时间持有锁）

---

## TD-18: JsonHelper 错误消息 path.string() — CJK 乱码

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | JsonHelper.cpp:L10/L25/L29 |

**描述**：`JsonHelper` 的错误消息使用 `path.string()` 拼接，在 Windows 上遇到 CJK 路径会乱码。注意：`std::ifstream file(path)` 和 `std::ofstream file(path)` 在 MSVC 上接受 `std::filesystem::path`，文件打开本身没问题，只是错误消息乱码。

**修复方案**：使用 `PathUtils::fromStdPath(path).toStdString()` 或 `path.u8string()` 构造错误消息，确保 CJK 路径正确显示。

**风险项**：低风险修复，与 TD-07 联动。

---

## 优先修复建议

1. **最优先**：TD-02（原始指针缺少 alive token）和 TD-14（后台线程调用非线程安全方法）— 可能导致用户数据损坏或应用崩溃
2. **次优先**：TD-03（AudioPlaybackManager 裸指针）和 TD-07（path.string() CJK 路径）— 影响用户体验和平台兼容性
3. **中期**：TD-01/TD-04/TD-13（QMetaMethod 反射替换为接口）和 TD-08（SpectrogramWidget 性能优化）— 架构改进
4. **低优先级**：TD-09/TD-10/TD-11/TD-12/TD-15/TD-16/TD-18 — 规范对齐和代码清理
