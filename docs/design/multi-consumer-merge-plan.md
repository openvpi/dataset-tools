# 多消费者相似代码排查与合并方案

> 生成日期：2026-05-12

## 一、排查范围

对 `src/apps/shared/data-sources/` 下的编辑页面类（PhonemeLabelerPage、PitchLabelerPage、MinLabelPage、SlicerPage）及框架核心模块进行了全面排查，重点关注相同行为多处实现的重复代码。

## 二、发现的重复模式

### 模式 1：引擎加载/确保模式 (ensure*Engine)

**涉及文件**：
- [PhonemeLabelerPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/PhonemeLabelerPage.cpp#L535-L559)
- [PitchLabelerPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/PitchLabelerPage.cpp#L310-L422)
- [MinLabelPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/MinLabelPage.cpp#L348-L405)

**重复内容**：每个页面都实现了 `ensure*Engine()` 和 `ensure*EngineAsync()`，遵循相同模式：

```
1. check if already loaded → return
2. get ModelManager
3. connect modelInvalidated signal
4. read config via readModelConfig()
5. loadModel() → get provider → cast to specific type
6. save engine pointer + alive token
```

**代码量**：每个引擎约 30 行，共 4 个引擎 × 2 方法 = ~240 行重复模式代码。

**合并方案**：
在 `EditorPageBase` 中添加模板方法 `ensureEngine<T>()`：

```cpp
template<typename TProvider, typename TEngine>
bool ensureEngine(TEngine *&enginePtr, 
                  std::shared_ptr<std::atomic<bool>> &alivePtr,
                  const QString &taskKey) {
    if (enginePtr && enginePtr->isOpen())
        return true;
    
    auto *mgr = ensureModelManager();
    if (!mgr) return false;
    
    if (!alivePtr) {
        connect(mgr, &IModelManager::modelInvalidated, 
                this, [this, &enginePtr, &alivePtr, taskKey](const QString &key) {
            if (key == taskKey) {
                if (alivePtr) alivePtr->store(false);
                enginePtr = nullptr;
                cancelAsyncTask(alivePtr);
            }
        });
    }
    
    auto config = readModelConfig(settingsBackend(), taskKey);
    if (config.modelPath.isEmpty()) return false;
    
    auto *mm = dynamic_cast<ModelManager *>(mgr);
    if (!mm) return false;
    
    auto result = mm->loadModel(taskKey, config, config.deviceId);
    if (!result) return false;
    
    auto typeId = registerModelType(taskKey);
    auto *provider = dynamic_cast<TProvider *>(mm->provider(typeId));
    if (provider && provider->engine() && provider->engine()->isOpen()) {
        enginePtr = provider->engine();
        alivePtr = std::make_shared<std::atomic<bool>>(true);
        return true;
    }
    return false;
}
```

**影响评估**：中等。需要各 Provider 统一 `engine()` 接口命名。

---

### 模式 2：单切片异步处理模式 (run*ForSlice)

**涉及文件**：
- [PhonemeLabelerPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/PhonemeLabelerPage.cpp#L584-L690) - `runFaForSlice`
- [PitchLabelerPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/PitchLabelerPage.cpp#L609-L676) - `runPitchExtraction`
- [PitchLabelerPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/PitchLabelerPage.cpp#L678-L791) - `runMidiTranscription`
- [MinLabelPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/MinLabelPage.cpp#L428-L488) - `runAsrForSlice`

**重复模式**：

```
1. validate source() and audioPath()
2. set m_xxxRunning = true
3. capture engine ptr + alive token + QPointer guard
4. QtConcurrent::run([...] {
   - check alive token
   - try { engine->process(...) } catch { log error }
   - QMetaObject::invokeMethod([guard, result] {
       - check guard
       - set m_xxxRunning = false
       - if success: apply result + toast
       - if error: log error + QMessageBox::warning
     })
   })
```

**代码量**：每个约 60-110 行，共约 350 行。

**合并方案**：
在 `EditorPageBase` 中提供一个通用异步处理框架：

```cpp
template<typename TEngine, typename TResult>
void runAsyncTask(const QString &taskName,
                  const QString &sliceId,
                  TEngine *engine,
                  std::shared_ptr<std::atomic<bool>> aliveToken,
                  std::atomic<bool> &runningFlag,
                  std::function<TResult(TEngine*, const QString&)> taskFunc,
                  std::function<void(const QString&, TResult)> applyFunc) {
    // validate, set flag, run, callback - unified logic
}
```

**影响评估**：较高。各页面的任务参数、返回类型差异较大，模板化需要仔细设计。建议保留当前结构，但抽取公共的 validate/guard/exception/callback 模式。

---

### 模式 3：`onAutoInfer` 模式

**涉及文件**：
- [PhonemeLabelerPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/PhonemeLabelerPage.cpp#L321-L372)
- [PitchLabelerPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/PitchLabelerPage.cpp#L424-L470)
- [MinLabelPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/MinLabelPage.cpp#L223-L234)

**重复模式**：

```
1. check hasExistingResult() → skip if present
2. ensure engines loaded (async)
3. wait for engines ready
4. run task
```

**合并方案**：
`onAutoInfer` 已通过虚函数 + `hasExistingResult()/prepareSliceInput()/processSlice()/applyBatchResult()` 体系在 `runBatchProcess` 中部分统一。子类的 `onAutoInfer` 仍各自实现了 ensure 和 wait 逻辑，可抽取到基类的 `autoInferWithEngines()` 模板中。

---

### 模式 4：MinLabelPage::onBatchAsr 重复实现 `runBatchProcess`

**涉及文件**：
- [EditorPageBase.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/EditorPageBase.cpp#L327-L438) - `runBatchProcess`（通用模板）
- [MinLabelPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/MinLabelPage.cpp#L490-L539+) - `onBatchAsr`（独立实现）

**严重程度**：**高** — 这是最严重的重复，`onBatchAsr` 完全重新实现了 `runBatchProcess` 的所有逻辑：对话框创建、进度更新、跳过已有检查、异步循环、错误计数、完成统计等。

其他页面（PhonemeLabelerPage、PitchLabelerPage）已迁移到 `runBatchProcess` 模板方法。

**合并方案**：
将 `onBatchAsr` 重构为使用 `EditorPageBase::runBatchProcess()`：

```cpp
void MinLabelPage::onBatchAsr() {
    // validate model / running state
    
    BatchConfig config;
    config.dialogTitle = tr("Batch ASR");
    config.skipExistingLabel = tr("Skip slices with existing lyrics");
    config.defaultSkipExisting = true;
    
    runBatchProcess(config, source()->sliceIds(),
        [this](BatchProcessDialog *dlg) {
            auto *autoG2PBox = new QCheckBox(tr("自动 G2P (拼音) 到结果"), dlg);
            autoG2PBox->setChecked(false);
            dlg->addParamWidget(autoG2PBox);
        });
}
```

需要实现 `hasExistingResult()`, `prepareSliceInput()`, `processSlice()`, `applyBatchResult()` 虚函数。

---

### 模式 5：引擎存活状态追踪 (Alive Token)

**涉及文件**：
- PhonemeLabelerPage: `m_hfaAlive`
- PitchLabelerPage: `m_rmvpeAlive`, `m_gameAlive`
- MinLabelPage: `m_asrAlive`

**重复模式**：每个引擎都维护 `std::shared_ptr<std::atomic<bool>>` 存活令牌，在 `onModelInvalidated` 中设为 false，在 `cancelAsyncTask` 中重置。

**合并方案**：
在 `EditorPageBase` 中提供一个 `EngineAliveToken` 工具类或使用 `aliveTokens` 映射：

```cpp
class EngineAliveToken {
    std::shared_ptr<std::atomic<bool>> m_token;
public:
    auto token() const { return m_token; }
    void invalidate() { if (m_token) m_token->store(false); m_token.reset(); }
    void create() { m_token = std::make_shared<std::atomic<bool>>(true); }
    explicit operator bool() const { return m_token && *m_token; }
};

std::map<QString, EngineAliveToken> m_aliveTokens;
```

**影响评估**：低。改动安全，能统一清理所有子类中的重复字段。

---

### 模式 6：结果应用与保存模式 (apply*Result)

**涉及文件**：
- `PhonemeLabelerPage::applyFaResult`
- `PitchLabelerPage`（apply通过 source()->writeSlice 直接保存）

**重复模式**：
```
1. source()->beginEdit(sliceId) / loadSlice
2. merge/update layers
3. source()->commitEdit(sliceId)
4. if currentSlice == sliceId: reload from source
5. markDirty/setDirty
```

当前已通过 `IEditorDataSource` 接口统一了大部分保存逻辑，但各页面仍有各自的 reload 和 dirty 标记代码。

**合并方案**：在 `EditorPageBase` 中添加 `applyAndReload()` 辅助方法。

---

## 三、合并优先级建议

| 优先级 | 模式 | 收益 | 风险 | 工作量 |
|--------|------|------|------|--------|
| **P0** | 4. MinLabelPage::onBatchAsr 独立循环 | 消除 ~80 行重复代码 | 低 | 小 |
| **P1** | 5. Alive Token 统一 | 4 处 → 1 处，统一管理 | 低 | 小 |
| **P1** | 6. applyAndReload 辅助方法 | 统一 3 个页面的 apply 模式 | 低 | 小 |
| **P2** | 1. ensureEngine 模板化 | ~240 行 → ~60 行 | 中（需统一 Provider 接口） | 中 |
| **P2** | 2. runAsyncTask 框架 | 可简化异步调用 | 高（参数差异大） | 大 |
| **P3** | 3. onAutoInfer 统一 | 减少样板代码 | 中 | 中 |

## 四、建议实施顺序

1. **先做 P0 + P1**：风险低、收益明确，可快速减少重复
2. **再做 P2**：需要设计通用接口，建议配合 Provider 接口重构一起做
3. **P3 最后**：等 P2 完成后自然简化 onAutoInfer 实现

## 五、设计原则校验

对照 [framework-architecture.md](file:///d:/projects/dataset-tools/docs/design/framework-architecture.md) 和 [unified-app-design.md](file:///d:/projects/dataset-tools/docs/design/unified-app-design.md)：

- **P-01 模块职责单一**：当前 4 个页面各自实现了相同的引擎加载模式，违反此原则。合并后每个模式只有一处实现 ✓
- **P-03 异步不阻塞UI**：已通过 QtConcurrent::run + QMetaObject::invokeMethod 统一实现 ✓
- **P-07 简单可靠**：合并方案保持现有设计风格，不引入新的复杂度 ✓
- **P-12 相似模块统一设计**：MinLabelPage::onBatchAsr 不符合此原则，需尽快统一 ✓