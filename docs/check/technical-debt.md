# 技术债审计报告

**审计日期**：2026-05-08
**验证日期**：2026-05-08（已对照源码逐项验证；新增 TD-19~TD-26）
**审计范围**：src/apps/、src/framework/、src/domain/、src/libs/
**参考文档**：conventions-and-standards.md、human-decisions.md

---

## 汇总

| 严重度 | 未修复 | 已修复 |
|--------|--------|--------|
| 高 | 5 | 3 |
| 中 | 10 | 4 |
| 低 | 5 | 0 |
| **合计** | **20** | **7** |

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

## TD-02: ~~PitchLabelerPage / MinLabelPage / ExportPage 原始指针缺少存活令牌~~ — ✅ 已修复

| 属性 | 值 |
|------|-----|
| **严重度** | ~~高~~ — 已修复 |
| **验证状态** | ✅ 已修复 |
| **修复日期** | 2026-05-08 |

**原描述**：根据 P-09 决策，所有 `QtConcurrent::run` 中捕获的原始指针必须配合 `std::shared_ptr<std::atomic<bool>>` 存活令牌使用。目前只有 `PhonemeLabelerPage` 正确实现了 `m_hfaAlive` 令牌模式，其他三个页面均未实现。

**修复验证**：
- PitchLabelerPage.h:57-58：已添加 `m_rmvpeAlive` 和 `m_gameAlive`
- MinLabelPage.h:45：已添加 `m_asrAlive`；MinLabelPage.h:57：已添加 `m_matchLyricAlive`
- ExportPage.h:69：已添加 `m_enginesAlive`（统一 token 模式，功能等价）

---

## TD-03: ~~AudioPlaybackManager 持有裸 QWidget 指针 — 潜在 use-after-free~~ — ✅ 已修复

| 属性 | 值 |
|------|-----|
| **严重度** | ~~高~~ — 已修复 |
| **验证状态** | ✅ 已修复 |
| **修复日期** | 2026-05-08 |

**原描述**：`AudioPlaybackManager::m_currentPlayer` 是一个裸 `QWidget*`，没有任何守卫。当 PlayWidget 被页面销毁，`m_currentPlayer` 变成悬空指针。

**修复验证**：AudioPlaybackManager.h:23 已改为 `QPointer<QWidget> m_currentPlayer`，悬空指针风险已消除。注意：AudioPlaybackManager.cpp 仍使用 `QMetaObject::invokeMethod(m_currentPlayer, "setPlaying", ...)` 字符串调用（属于 TD-13 范畴）。

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

## TD-07: path.string() 在 Windows 上破坏 CJK 路径 — 违反 §12（部分修复）

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ⚠️ 部分修复 |
| **影响文件** | Hfa.cpp, DictionaryG2P.cpp, GameModel.cpp, JsonUtils.h, Util.cpp（仍存在）；~~SlicerService.cpp, JsonHelper.cpp~~（已修复） |

**描述**：根据项目规范 §12，推理层代码中禁止使用 `path.string()` 进行文件 I/O（Windows 上会破坏 CJK 字符）。

**已修复**：
- ~~SlicerService.cpp~~ — 已改用 `AudioUtil::openSndfile(path)`
- ~~JsonHelper.cpp~~ — 已移除 `path.string()` 错误消息

**仍存在**（按严重程度排序）：
1. **Hfa.cpp** — `DictionaryG2P(dict_path.string(), language)` 传给 DictionaryG2P 构造函数做文件 I/O，Windows 上 CJK 路径打开失败
2. **DictionaryG2P.cpp** — 构造函数接收 `const std::string &dictionaryPath`，内部 `std::ifstream file(dictionaryPath)` 打开文件
3. **GameModel.cpp** — 错误消息仍使用 `path.string()` 乱码（ONNX Session 创建已正确使用 wstring）
4. **JsonUtils.h** — 5 处 `path.string()` 拼接错误消息
5. **Util.cpp** — `filepath.string()` 拼接错误消息

**修复方案**：
1. `DictionaryG2P` 构造函数参数改为 `std::filesystem::path`，内部用 `std::ifstream(path)` 打开。调用处改为 `DictionaryG2P(dict_path, language)`
2. 所有错误消息中使用 `PathUtils::fromStdPath()` 或 `path.u8string()` 转换

**风险项**：
- 需要区分 Windows 和 Unix 平台的路径转换逻辑
- 修改 `DictionaryG2P` 接口可能影响其他调用者

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

## TD-18: ~~JsonHelper 错误消息 path.string() — CJK 乱码~~ — ✅ 已修复

| 属性 | 值 |
|------|-----|
| **严重度** | ~~中~~ — 已修复 |
| **验证状态** | ✅ 已修复 |
| **修复日期** | 2026-05-08 |

**原描述**：`JsonHelper` 的错误消息使用 `path.string()` 拼接，在 Windows 上遇到 CJK 路径会乱码。

**修复验证**：JsonHelper.cpp 已移除所有 `path.string()` 调用，错误消息不再乱码。

---

## TD-19: ~~PitchLabelerPage::onSliceSelectedImpl 音频路径验证逻辑不一致~~ — ✅ 已修复

| 属性 | 值 |
|------|-----|
| **严重度** | ~~中~~ — 已修复 |
| **违反原则** | P-04（错误根因传播） |
| **验证状态** | ✅ 已修复（BUG-26 + ARCH-16 统一） |
| **修复日期** | 2026-05-08 |

**原描述**：`onSliceSelectedImpl()` 使用 `audioPath()`（可能不存在）而非 `validatedAudioPath()`（保证文件存在），与 PhonemeLabelerPage 的做法不一致。

**修复方案**：BUG-26 修复中改用 `validatedAudioPath()`；ARCH-16 统一了所有页面的音频加载模式。

---

## TD-20: ~~PitchEditor::loadAudio duration=0.0 导致 PianoRollView 视口总时长未设置~~ — ✅ 已修复

| 属性 | 值 |
|------|-----|
| **严重度** | ~~中~~ — 已修复 |
| **违反原则** | P-04 |
| **验证状态** | ✅ 已修复（BUG-26 + ARCH-16 统一） |
| **修复日期** | 2026-05-08 |

**原描述**：`PitchLabelerPage::onSliceSelectedImpl()` 调用 `m_editor->loadAudio(audioPath, 0.0)` 硬编码 `duration=0.0`。`PitchEditor::loadAudio()` 中 `if (duration > 0) m_pianoRoll->setAudioDuration(duration)` 因此永远不执行。

**修复方案**：BUG-26 修复中从 `doc.audio` 计算时长；ARCH-16 统一使用 `audioDurationSec()` 辅助方法。

---

## TD-21: ~~LayerDependency.parentLayerIndex/childLayerIndex 使用硬编码索引~~ — ✅ 已修复

| 属性 | 值 |
|------|-----|
| **严重度** | ~~中~~ — 已修复 |
| **违反原则** | P-06（接口稳定） |
| **验证状态** | ✅ 已修复 |
| **影响文件** | PhonemeLabelerPage.cpp, DsTextTypes.h, DsTextDocument.cpp |
| **修复日期** | 2026-05-08 |

**原描述**：`buildFaLayers()` 中 `LayerDependency` 的 `parentLayerIndex = 0` 和 `childLayerIndex = 1` 是硬编码值，假设 grapheme 层总是第一个、phoneme 层总是第二个。当文档中层的顺序变化（如存在 `raw_text` 层或其他层）时，这些索引可能指向错误的层。

**修复方案**：在 `LayerDependency` 中添加 `parentLayerName`/`childLayerName` 字段，`buildFaLayers()` 设置名称而非索引，`applyFaResult()` 根据名称解析为实际索引。序列化格式新增 `parentName`/`childName` 键，向后兼容旧文件。

---

## TD-22: ~~QtConcurrent::run lambda 中推理调用无 try-catch 保护 — 异常致崩溃~~ — ✅ 已修复

| 属性 | 值 |
|------|-----|
| **严重度** | ~~高~~ — 已修复 |
| **违反原则** | P-05（异常边界隔离） |
| **验证状态** | ✅ 已修复 |
| **修复日期** | 2026-05-08 |
| **影响文件** | PitchLabelerPage.cpp, PhonemeLabelerPage.cpp, MinLabelPage.cpp, EditorPageBase.cpp |

**描述**：多个页面的 `QtConcurrent::run` lambda 中直接调用推理方法（`rmvpe->get_f0()`、`game->getNotes()`、`hfa->recognize()`、`asr->recognize()`、`matchLyric->matchText()`），这些方法内部可能抛出 `Ort::Exception` 或 `std::exception`。lambda 没有 try-catch 保护，如果异常逃逸出 lambda，`QtConcurrent::run` 将导致 `std::terminate()` 崩溃。

**对比**：ExportPage.cpp:627 的 lambda 已有 try-catch 保护（`catch (const std::exception &e)` + `DSFW_LOG_ERROR`），是正确做法。

**修复方案**：在所有推理 lambda 外层添加 try-catch，参照 ExportPage 的模式：

```cpp
QtConcurrent::run([engine, alive, ...]() {
    try {
        if (!alive || !*alive) return;
        auto result = engine->infer(...);
        // ...
    } catch (const Ort::Exception &e) {
        DSFW_LOG_ERROR("infer", e.what());
    } catch (const std::exception &e) {
        DSFW_LOG_ERROR("infer", e.what());
    }
});
```

**风险项**：
- 需要逐一修改约 9 处 lambda
- 需要确保所有推理调用都在 try-catch 保护下
- EditorPageBase::loadEngineAsync 的 `loadFunc()` 也需要保护

---

## TD-23: ExportFormats.cpp 错误消息使用 path.string() — CJK 乱码

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反原则** | §12（路径 I/O 规范） |
| **验证状态** | ✅ 已确认 |
| **影响文件** | ExportFormats.cpp:L69 |

**描述**：`SinsyXmlExportFormat::exportItem()` 中 `return Err("Cannot open output file: " + outputPath.string())` 使用 `path.string()` 构造错误消息，Windows CJK 路径下会乱码。

**修复方案**：使用 `outputPath.u8string()` 或 `PathUtils::fromStdPath(outputPath)` 构造错误消息。

**风险项**：低风险修复。

---

## TD-24: 硬编码中文字符串未使用 tr() 包裹 — i18n 缺陷（TD-09 大幅扩展）

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反原则** | §4 日志规范、i18n 规范 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | PitchLabelerPage.cpp (~45处), SlicerPage.cpp (~30处), DsSlicerPage.cpp (~30处), ExportPage.cpp (~40处), PitchEditor.cpp (~20处), PianoRollView.cpp (~20处), PropertyPanel.cpp (~15处), SettingsPage.cpp (~60处), SliceListPanel.cpp (~5处), TextGridDocument.cpp (1处), DsLabeler/main.cpp (~15处) |

**描述**：TD-09 仅记录了 PitchLabelerPage.cpp 的 4 处 `QStringLiteral` 硬编码中文，但实际范围远大于此。全文搜索发现约 **250+ 处** `QStringLiteral` 硬编码中文字符串未使用 `tr()` 包裹，涉及 11 个文件。这意味着这些字符串无法被 Qt Linguist 提取翻译，应用无法支持多语言。

**修复方案**：将所有 `QStringLiteral("中文...")` 替换为 `tr("中文...")`，并更新 `.ts` 翻译文件。建议按页面逐步迁移，优先迁移用户可见的对话框和菜单文本。

**风险项**：
- 工作量较大（250+ 处）
- 需要确保所有包含 tr() 的类都继承 QObject 并有 Q_OBJECT 宏
- 需要更新翻译文件

---

## TD-25: domain 层 catch 块缺少 DSFW_LOG 日志记录

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **违反原则** | P-05（异常边界隔离） |
| **验证状态** | ✅ 已确认 |
| **影响文件** | DsTextDocument.cpp:L34, QualityMetrics.cpp:L29/L48, ExportFormats.cpp:L35/L53/L85/L106, DsItemManager.cpp:L37/L113 |

**描述**：domain 层多个 catch 块仅返回 `Result::Error()` 但不记录 DSFW_LOG 日志。虽然通过 `Result<T>` 传播错误不违反 P-05，但缺少日志记录不利于生产环境调试——FileLogSink 不会记录这些错误。

**修复方案**：在 catch 块中添加 `DSFW_LOG_ERROR("io", ...)` 日志记录。

**风险项**：低风险修复。

---

## TD-26: 多处使用 qWarning/qDebug 而非 DSFW_LOG — 日志绕过

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **违反原则** | §4 日志规范 |
| **验证状态** | ✅ 已确认（9 处新增） |
| **影响文件** | AsrPipeline.cpp:L39, MatchLyric.cpp:L30, ProjectDataSource.cpp:L204/L210/L219, DsDocument.cpp:L45, BatchCheckpoint.cpp:L89, AppPaths.cpp:L63/L65 |

**描述**：LOG-03 已记录了 15+ 处 qWarning/qDebug/qCritical 使用，本次排查新增 9 处。这些日志绕过统一日志系统，不会写入 FileLogSink，无法在生产环境中追溯。

**修复方案**：统一替换为 `DSFW_LOG_WARN/DEBUG/ERROR` 宏，使用正确的分类字符串。

**风险项**：低风险修复，工作量较小。

---

## 优先修复建议

1. **最优先**：TD-22（QtConcurrent::run lambda 无 try-catch）— 模型加载失败或 GPU OOM 时直接崩溃，无任何错误提示
2. **次优先**：TD-14/BUG-11（后台线程调用非线程安全方法）— 可能导致用户数据损坏或应用崩溃；新增 PitchLabelerPage:892、MinLabelPage:540、ExportService:83 遗漏调用点
3. **重要**：TD-07（path.string() CJK 路径 — 剩余 Hfa/DictionaryG2P/GameModel/JsonUtils/Util/ExportFormats）— 影响平台兼容性
4. **中期**：TD-01/TD-04/TD-13（QMetaMethod 反射替换为接口）和 TD-08（SpectrogramWidget 性能优化）— 架构改进
5. **中期**：TD-24（硬编码中文 tr() 迁移）— 250+ 处，建议按页面逐步迁移
6. **低优先级**：TD-05/TD-06/TD-09/TD-10/TD-11/TD-12/TD-15/TD-16/TD-17/TD-23/TD-25/TD-26 — 规范对齐和代码清理
