# 未实现接口与功能桩

## 一、Core层接口桩 (仅有Stub实现, 无生产代码)

### 1. IExportFormat → StubExportFormat
- 头文件: src/core/include/dstools/IExportFormat.h
- 用途: 数据集导出格式扩展点 (如导出为HTS Label, Sinsy XML等)
- 当前状态: StubExportFormat::exportData() 返回错误 "Export not implemented"
- 缺失的生产实现: 无任何具体格式的导出器

### 2. IG2PProvider → StubG2PProvider
- 头文件: src/core/include/dstools/IG2PProvider.h
- 用途: Grapheme-to-Phoneme转换抽象层
- 当前状态: StubG2PProvider::convert() 返回空结果
- 说明: 实际G2P功能在MinLabel中通过cpp-pinyin直接调用, 未经过此接口. 接口设计存在但未被集成.

### 3. IModelDownloader → StubModelDownloader
- 头文件: src/core/include/dstools/IModelDownloader.h
- 用途: 模型自动下载 (从GitHub Releases等)
- 当前状态: StubModelDownloader::download() 返回错误 "Download not available"
- 说明: 用户当前需手动下载模型文件

### 4. IQualityMetrics → StubQualityMetrics
- 头文件: src/core/include/dstools/IQualityMetrics.h
- 用途: 数据集质量评分 (SNR, 对齐置信度等)
- 当前状态: StubQualityMetrics::evaluate() 返回空指标
- 说明: 完全未实现, 无UI入口

### 5. IStepPlugin → StubStepPlugin
- 头文件: src/core/include/dstools/IStepPlugin.h
- 用途: Pipeline步骤插件系统 (允许第三方扩展处理步骤)
- 当前状态: StubStepPlugin::execute() 返回错误
- 说明: 插件架构已搭建但从未使用, TranscriptionPipeline直接硬编码步骤

## 二、DiffSingerLabeler 页面桩

### 6. BuildCsvPage (步骤6: 构建CSV)
- 文件: src/apps/labeler/pages/BuildCsvPage.cpp L59-63
- 当前: runSlot() 弹出 "Not Implemented" 对话框
- 预期功能: 从TextGrid + 音频生成训练用CSV

### 7. BuildDsPage (步骤7: 构建DS)
- 文件: src/apps/labeler/pages/BuildDsPage.cpp L72-77
- 当前: runSlot() 弹出 "Not Implemented" 对话框
- 预期功能: 从CSV生成.ds文件 (调用CsvToDsConverter)

### 8. GameAlignPage (步骤8: MIDI对齐)
- 文件: src/apps/labeler/pages/GameAlignPage.cpp L58-63
- 当前: runSlot() 弹出 "Not Implemented" 对话框
- 预期功能: GAME模型对齐集成

### 9. Placeholder步骤 (步骤0,1,3)
- 文件: src/apps/labeler/LabelerWindow.cpp L219-245
- 当前: 步骤0(Slice)、步骤1(ASR)、步骤3(Align) 只有QLabel占位符, 无实际页面
- 预期功能: 分别集成SlicerPage、LyricFAPage、HubertFAPage

### 10. IPageProgress 接口
- 文件: src/apps/labeler/IPageProgress.h
- 当前: 定义了5个虚方法(progressTotal, progressCurrent, isRunning, progressMessage, cancelOperation), 无任何类实现此接口
- 预期功能: 进度条和取消操作

## 三、PitchLabeler Undo/Redo

### 11. onUndo() / onRedo() / updateUndoRedoState()
- 文件: src/apps/PitchLabeler/gui/PitchLabelerPage.cpp L606-614
- 当前: 三个方法体为空, 注释标记 "TODO: undo not yet implemented"
- 说明: m_actUndo和m_actRedo action已创建并绑定快捷键, 但触发后无任何效果

## 四、PhonemeLabeler/PitchLabeler空方法

### 12. updatePlaybackState() - PhonemeLabeler
- 文件: src/apps/PhonemeLabeler/gui/PhonemeLabelerPage.cpp L573-575
- 当前: 空方法体

### 13. updatePlaybackState() - PitchLabeler
- 文件: src/apps/PitchLabeler/gui/PitchLabelerPage.cpp L673-675
- 当前: 空方法体

## 五、Core F0提取桩

### 14. CsvToDsConverter F0 extraction
- 文件: src/core/src/CsvToDsConverter.cpp L103
- 当前: `// TODO: F0 extraction stub — if f0Callback is null, f0 stays empty`
- 说明: 如果不提供f0Callback, 生成的.ds文件中f0_seq为空, 后续PitchLabeler需要手动编辑

## 六、FunAsr GPU加速

### 15. FunAsr无DML/CUDA支持
- 位置: src/infer/FunAsr/src/paraformer_onnx.cpp
- 当前: 仅CPU推理, 无任何ExecutionProvider配置
- 其他推理库(GAME, SOME, RMVPE, HuBERT)均支持DML/CUDA
