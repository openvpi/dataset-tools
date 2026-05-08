# Bug 与 Issue 审计报告

**审计日期**：2026-05-08
**验证日期**：2026-05-08（已对照源码逐项验证）
**审计范围**：src/ 全目录
**参考文档**：conventions-and-standards.md、human-decisions.md

---

## 汇总

| 严重度 | 数量 |
|--------|------|
| 高 | 6 |
| 中 | 8 |
| 低 | 2 |
| **合计** | **16** |

> 注：初版 BUG-17/18（ServiceLocator::get 未做空指针检查）经源码验证后发现已有空指针检查，已移除。

---

## BUG-01: PitchLabelerPage 缺少推理引擎存活令牌（Use-After-Free）

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **违反原则** | P-09 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | PitchLabelerPage.cpp:L514/L566/L824, PitchLabelerPage.h:L53-54 |

**描述**：`PitchLabelerPage` 在 `QtConcurrent::run` 的 lambda 中捕获了 `m_rmvpe` 和 `m_game` 的原始指针副本，但没有对应的存活令牌（对比 `PhonemeLabelerPage` 已正确实现 `m_hfaAlive`：L446 初始化，L553-554 检查，L251 store(false) + cancelAsyncTask）。仅使用 `QPointer<PitchLabelerPage>` 作为 guard，但这只保护页面对象本身，不保护引擎指针有效性。

**复现场景**：在 PitchLabelerPage 执行音高提取时，切换到 Settings 页面更改模型配置触发 `modelInvalidated` 信号，后台线程继续使用已销毁的引擎。

**修复方案**：为 `m_rmvpe` 和 `m_game` 各添加 `std::shared_ptr<std::atomic<bool>>` 存活令牌，在 `onModelInvalidated` 中置 false 并 reset，在 lambda 中检查令牌后再使用引擎。

**风险项**：
- 修复后仍存在 TOCTOU 间隙（见 TD-17），但已大幅降低崩溃概率
- 需要确保 `onModelInvalidated` 槽在引擎销毁前执行

---

## BUG-02: MinLabelPage 缺少 ASR 引擎存活令牌（Use-After-Free）

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **违反原则** | P-09 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | MinLabelPage.cpp:L418/L495/L728, MinLabelPage.h:L42 |

**描述**：与 BUG-01 相同的问题。`MinLabelPage` 在 `QtConcurrent::run` 中捕获 `m_asr` 原始指针，但没有存活令牌。`onModelInvalidated` 仅设置 `m_asr = nullptr`，不会阻止后台线程使用已销毁的 ASR 引擎。

**复现场景**：批量 ASR 运行期间，切换到 Settings 页面更改模型配置。

**修复方案**：同 BUG-01，添加 `m_asrAlive` 存活令牌。

**风险项**：同 BUG-01。

---

## BUG-03: ExportPage 后台任务持有引擎原始指针，onDeactivated 直接销毁引擎（Use-After-Free）

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **违反原则** | P-09 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | ExportPage.cpp:L588-596, L997-1002 |

**描述**：`ExportPage::onExport()` 在 `QtConcurrent::run` 中捕获 `m_hfa.get()`、`m_rmvpe.get()`、`m_game.get()` 的原始指针。但 `onDeactivated()` 直接 `m_hfa.reset()` / `m_rmvpe.reset()` / `m_game.reset()` 销毁引擎。注释"Engines are owned by ExportPage and persist until deactivation"说明了设计意图，但并未保证后台任务在 deactivation 前完成。

**复现场景**：导出页面勾选"自动补全"开始导出，立即切换到其他页面，触发 `onDeactivated()`。

**修复方案**：添加存活令牌；或在 `onDeactivated` 中等待后台任务完成后再销毁引擎；或使用 `QPointer` 守卫 + 原子标志。

**风险项**：
- 等待后台任务完成可能导致页面切换卡顿
- 需要设计取消机制

---

## BUG-04: HFA 字典加载使用 path.string() 导致 Windows CJK 路径失败

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **违反原则** | §12（路径 I/O 规范） |
| **验证状态** | ✅ 已确认 |
| **影响文件** | Hfa.cpp:L62, DictionaryG2P.cpp:L9-13 |

**描述**：`HFA::load()` 中 `m_dictG2p[language] = std::make_unique<DictionaryG2P>(dict_path.string(), language)` 将 `std::filesystem::path` 转为 `std::string` 传给 `DictionaryG2P` 构造函数。`DictionaryG2P` 构造函数（L9）接收 `const std::string &dictionaryPath`，内部 `std::ifstream file(dictionaryPath)`（L11）打开文件。在 Windows 上，`path.string()` 返回 ANSI 编码字符串，`std::ifstream(const std::string&)` 也使用 ANSI 编码，导致包含 CJK 字符的模型路径无法打开字典文件。

**复现场景**：将模型放在含中文路径的目录下（如 `D:\模型\hubert\`），尝试加载 HFA 模型。

**修复方案**：将 `DictionaryG2P` 构造函数参数改为 `std::filesystem::path`，内部使用 `std::ifstream(path)` 直接接受 path 对象（MSVC 支持）。调用处改为 `DictionaryG2P(dict_path, language)`。

**风险项**：
- 需要修改 DictionaryG2P 的接口
- 需确认 GCC/Clang 也支持 `std::ifstream(path)` 构造

---

## BUG-05: GameModel::loadModel 使用 path.string() 构造错误消息

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **违反原则** | §12 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | GameModel.cpp:L40/L84 |

**描述**：第 40 行 `msg = "Could not open config.json: " + (modelPath / "config.json").string()` 和第 84 行 `msg = model_path.string() + " not exist!"` 使用 `path.string()` 构造错误消息，在 Windows CJK 路径下会产生乱码。注意：L88-91 ONNX Session 创建已正确使用 `model_path.wstring().c_str()` (Windows) / `model_path.c_str()` (非 Windows)。

**修复方案**：错误消息使用 `path.u8string()` 或 `QString::fromStdWString(path.wstring()).toStdString()` 来正确编码。

**风险项**：低风险修复。

---

## BUG-06: JsonUtils::loadFile 使用 path.string() 构造错误消息

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反原则** | §12 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | JsonUtils.h:L47-66 |

**描述**：`JsonUtils::loadFile()` 在多处错误消息中使用 `path.string()`。虽然 `std::ifstream stream(path)` 本身正确（MSVC 接受 path 对象），但错误消息在 Windows CJK 路径下会乱码。

**修复方案**：错误消息改用 `path.u8string()` 或 UTF-8 编码方式。

**风险项**：低风险修复。

---

## BUG-07: TextGridDocument::clampBoundaryTime 与 moveBoundary 行为不一致

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | TextGridDocument.cpp:L323-349, L156-186 |

**描述**：`clampBoundaryTime`（L323-349）和 `moveBoundary`（L156-186）对同一操作使用不同的最小间隔值，导致行为不一致：

- `clampBoundaryTime`：`minClamp = prevBoundary`（不加 epsilon），`maxClamp = nextBoundary - kEpsilon`（kEpsilon = 0.000001）
- `moveBoundary`：`std::clamp(newTimeSec, prevBoundary + kMinInterval, nextBoundary - kMinInterval)`（kMinInterval = 0.001）

这意味着拖拽时 `clampBoundaryTime` 允许的位置（最小间隔 0.000001 秒）与实际移动后 `moveBoundary` 允许的位置（最小间隔 0.001 秒）不同，用户可能拖到某个位置后松手发现边界跳回。

**修复方案**：统一使用 `kMinInterval = 0.001`，并在 `minClamp` 侧也加上间隔：`minClamp = prevBoundary + kMinInterval`。

**风险项**：需确认是否所有场景下零宽度 interval 都不可接受。

---

## BUG-08: SliceBoundaryModel::boundaryCount 不包含首尾边界

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | SliceBoundaryModel.cpp:L22-26 |

**描述**：`SliceBoundaryModel::boundaryCount()` 返回 `m_pointsSec.size()`（切片点数量），而 `TextGridDocument::boundaryCount()` 返回 `intervalCount + 1`（包含首尾边界）。`SliceBoundaryModel` 只存储内部切点，不包含 0 和 duration 的首尾边界。依赖 `IBoundaryModel::boundaryCount()` 的代码在 `SliceBoundaryModel` 上会遗漏首尾边界线。

**修复方案**：确认 Slicer 页面是否需要首尾边界线。如果需要，`SliceBoundaryModel::boundaryCount()` 应返回 `m_pointsSec.size() + 2`（包含 0 和 duration），并在 `boundaryTime()` 中相应处理。

**风险项**：需要审查所有 `IBoundaryModel` 消费者是否正确处理语义差异。

---

## BUG-09: MinLabelPage::createStatusBarContent 未使用 StatusBarBuilder

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反原则** | D-39 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | MinLabelPage.cpp:L270-285 |

**描述**：`MinLabelPage::createStatusBarContent()` 手动创建布局和标签，并使用裸 `connect()` 而非 `StatusBarBuilder`，不符合 D-39 规定。

**修复方案**：迁移到 `StatusBarBuilder` 模式。

**风险项**：低风险修复。

---

## BUG-11: ExportPage 后台线程调用 source()->loadSlice() 无线程安全保护

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **验证状态** | ✅ 已确认 |
| **影响文件** | ExportPage.cpp:L606, MinLabelPage.cpp:L742, PitchLabelerPage.cpp:L848 |

**描述**：多个页面的批量处理 `QtConcurrent::run` lambda 中调用了 `src->loadSlice(sliceId)`。`IEditorDataSource::loadSlice()` 没有任何线程安全文档或保证（IEditorDataSource.h:L38），其实现可能访问非线程安全的资源。

**验证补充**：
- ExportPage.cpp:L837 — `saveSlice` 通过 `QMetaObject::invokeMethod` 回到主线程（正确），但 `loadSlice` 在后台线程（不一致）
- 如果主线程同时进行保存操作（自动保存定时器），将产生数据竞争

**复现场景**：批量 ASR 运行期间，自动保存定时器触发 `saveCurrentSlice()`，两者同时访问 `IEditorDataSource` 的内部数据结构。

**修复方案**：
1. 在 `IEditorDataSource` 文档中明确标注 `loadSlice()` / `saveSlice()` 是否线程安全
2. 如果不线程安全，后台线程应通过 `QMetaObject::invokeMethod` 回到主线程执行 `loadSlice()`
3. 或者让 `ProjectDataSource` 内部加锁保证线程安全

**风险项**：
- 预加载方案可能增加内存占用
- mutex 方案可能降低并发性能
- 需要审查 `IEditorDataSource` 的所有实现

---

## BUG-12: JsonHelper::get() 使用 catch(...) 吞掉所有异常

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反原则** | P-05 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | JsonHelper.h:30 |

**描述**：`JsonHelper::get<T>()` 方法使用 `catch (...)` 捕获所有异常后静默返回默认值，违反 P-05。

**修复方案**：至少添加 `DSFW_LOG_WARN` 日志记录，或改为返回 `Result<T>` 让调用者处理。

**风险项**：改为 `Result<T>` 会影响所有调用点。

---

## BUG-13: ExportPage::ensureEngines 在主线程同步加载多个 ONNX 模型

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反原则** | P-03 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | ExportPage.cpp:L305-358 |

**描述**：`ensureEngines()` 在主线程同步加载 HFA、RMVPE、GAME 三个 ONNX 模型，每个模型加载可能耗时数秒，总共可能阻塞 UI 5-15 秒。对比 PhonemeLabelerPage 的 `ensureHfaEngineAsync()` 和 PitchLabelerPage 的 `ensureRmvpeEngineAsync()` / `ensureGameEngineAsync()` 已使用异步加载。

**修复方案**：将 `ensureEngines()` 改为异步版本，使用 `QTimer::singleShot` 或 `QtConcurrent::run` + `QMetaObject::invokeMethod` 模式，加载完成后通过回调继续导出流程。

**风险项**：需要设计加载状态 UI（进度指示器）。

---

## BUG-14: TextGridDocument::autoDetectBindingGroups 使用合成 ID 方案，边界插入/删除后失效

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | TextGridDocument.cpp:L493-524 |

**描述**：`autoDetectBindingGroups()` 使用 `uniqueId = loc.tier * 10000 + loc.index + 1` 作为跨层边界 ID。当边界被插入或删除后，`loc.index` 会偏移，导致合成 ID 指向错误的边界。虽然 D-27 决定运行时 binding 改为时间匹配驱动（不依赖合成 ID），但 `autoDetectBindingGroups()` 仍用于序列化到 `.dstext` 文件。

**修复方案**：在 `insertBoundary` / `removeBoundary` 后重新调用 `autoDetectBindingGroups()` 更新 groups，或在序列化时基于时间位置而非索引生成 ID。

**风险项**：D-27 完成后此问题影响范围将缩小（仅影响序列化）。

---

## BUG-15: AudioVisualizerContainer::connectViewportToWidget 使用反射调用 setViewport

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | AudioVisualizerContainer.cpp:L650-658 |

**描述**：`connectViewportToWidget()` 通过 `QMetaObject::methodCount()` 遍历查找名为 `setViewport` 的方法，然后使用 `QMetaMethod::invoke()` 调用。性能开销大，且 `method.parameterTypes().at(0)` 没有越界检查。

**修复方案**：改为直接 connect 到 widget 的 `setViewport` 槽函数，或定义一个 `IViewportAware` 接口。

**风险项**：与 TD-01 联动。

---

## BUG-16: PipelineContext::fromJson 使用 j.at() 无异常保护

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反原则** | P-05 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | PipelineContext.cpp:L98-145 |

**描述**：`PipelineContext::fromJson()` 大量使用 `j.at("key")` 访问 JSON 字段。`nlohmann::json::at()` 在键不存在或类型不匹配时会抛出异常。根据 P-05，应该在第三方库边界捕获异常并转为 `Result<T>::Error()`。

**修复方案**：在 `fromJson` 外层添加 try-catch 将 `nlohmann::json::exception` 转为 `Result<PipelineContext>::Error()`，或对每个 `at()` 调用改用 `value()` 方法提供默认值。

**风险项**：低风险修复。

---

## BUG-19: ExportPage 后台线程中推理调用无异常保护

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反原则** | P-05 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | ExportPage.cpp:L666 |

**描述**：`ExportPage::onExport()` 的 `QtConcurrent::run` lambda 中直接调用 `hfa->recognize()`、`rmvpe->get_f0()`、`game->getNotes()` 等推理方法。这些方法内部可能抛出 `Ort::Exception` 或其他异常。lambda 没有 try-catch 保护，如果异常逃逸出 lambda，`QtConcurrent::run` 将导致程序崩溃（未捕获异常终止）。

**复现场景**：模型文件损坏或 GPU 内存不足时，ONNX Runtime 抛出异常。

**修复方案**：在 lambda 外层添加 try-catch，捕获异常后通过 `QMetaObject::invokeMethod` 在主线程显示错误消息。

**风险项**：低风险修复，但需确保所有推理调用都在 try-catch 保护下。

---

## 按类别汇总

| 类别 | 数量 | ID 列表 |
|------|------|---------|
| Use-After-Free / 悬空指针 | 3 | BUG-01, BUG-02, BUG-03 |
| Windows CJK 路径问题 | 3 | BUG-04, BUG-05, BUG-06 |
| 竞态条件 / 线程安全 | 2 | BUG-11, BUG-19 |
| 边界计算 / 行为不一致 | 2 | BUG-07, BUG-08 |
| 错误传播 / 异常处理 | 3 | BUG-12, BUG-16, BUG-19 |
| 生命周期 / 信号连接 | 1 | BUG-09 |
| 异步规范违反 | 1 | BUG-13 |
| 数据一致性 | 1 | BUG-14 |
| 性能 / 设计问题 | 1 | BUG-15 |

---

## 优先修复建议

1. **最紧急**：BUG-01, BUG-02, BUG-03（use-after-free，可能导致 0xC0000005 崩溃）
2. **次紧急**：BUG-04, BUG-05（Windows CJK 路径，功能性缺陷，中文用户完全无法使用 HFA）
3. **重要**：BUG-11, BUG-19（线程安全，批量操作场景下易触发）
4. **改善**：BUG-07, BUG-12, BUG-13, BUG-16（错误处理和异步规范）
