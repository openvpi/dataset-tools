# 任务清单 (Task Backlog)

## 优先级说明
- P0: 阻塞性问题, 影响稳定性/正确性
- P1: 重要功能缺失或严重技术债
- P2: 改善项, 提升可维护性
- P3: Nice-to-have

---

## P1 任务

### TASK-005: 添加macOS/Linux CI
- **目标**: 三平台构建验证
- **实现步骤**:
  1. 创建.github/workflows/macos-build.yml (Qt6 + vcpkg + arm64-osx)
  2. 创建.github/workflows/linux-build.yml (Qt6 + vcpkg + x64-linux)
  3. PR触发 + tag触发
  4. 启用BUILD_TESTS=ON + ctest步骤
- **风险**: 高 — FunAsr在非Windows可能构建失败
- **评估**: CI绿色; 三平台构建产物可运行

### TASK-007: 统一推理库EP初始化
- **目标**: 消除5处重复的DML/CUDA init代码
- **实现步骤**:
  1. 确认dstools-infer-common的OnnxEnv API满足所有推理库需求
  2. 修改game-infer: 删除本地initDirectML/initCUDA, 链接infer-common, 使用OnnxEnv
  3. 对some-infer, rmvpe-infer, hubert-infer重复步骤2
  4. FunAsr: 也切换到OnnxEnv (同时获得GPU支持)
  5. 删除各库中的Ort::Env成员, 改用OnnxEnv::env()
- **风险**: 高 — 改动所有推理库的核心初始化路径, 需全量测试
- **评估**: 所有TestXxx通过; GPU推理行为不变

### TASK-009: 实现DiffSingerLabeler步骤0/1/3集成
- **目标**: 将pipeline页面集成到labeler向导
- **实现步骤**:
  1. 步骤0: 创建SliceLabelerPage, 复用SlicerPage核心逻辑
  2. 步骤1: 创建AsrLabelerPage, 复用LyricFAPage核心逻辑
  3. 步骤3: 创建AlignLabelerPage, 复用HubertFAPage核心逻辑
  4. 实现IPageLifecycle接口(onActivated/onDeactivated)
  5. 连接IPageProgress信号到StepNavigator进度条
- **风险**: 高 — 需要重构pipeline页面使其可嵌入, 或抽取共享组件
- **评估**: 9步向导全部可操作, 文件在步骤间正确传递

### TASK-010: 实现PitchLabeler Undo/Redo
- **目标**: F0编辑支持撤销/重做
- **实现步骤**:
  1. 引入QUndoStack (PhonemeLabeler已有先例)
  2. 为每种编辑工具(Select/Modulation/Drift)创建QUndoCommand子类
  3. 在编辑操作中push command而非直接修改数据
  4. onUndo()/onRedo()调用undo()/redo()
  5. updateUndoRedoState()根据canUndo()/canRedo()更新action状态
- **风险**: 中 — 需理解PianoRollView的数据模型
- **评估**: 编辑→Ctrl+Z→恢复原状; Ctrl+Y→重新应用

---

## P2 任务

### TASK-011: 抽取CMake infer公共模板
- **目标**: 消除5处CMake模板重复
- **实现步骤**:
  1. 创建cmake/infer-target.cmake, 定义dstools_add_infer_library()宏
  2. 宏封装: MSVC flags, output dirs, ORT linking, DML/CUDA defs, install/export
  3. 各库CMakeLists.txt改为调用此宏
- **风险**: 低 — 纯构建系统重构
- **评估**: 全平台cmake configure + build成功

### TASK-012: 统一错误报告机制
- **目标**: 消除std::string/QString error参数混用
- **实现步骤**:
  1. 定义dstools::Result<T>模板 (类似std::expected)
  2. 逐步迁移core层函数返回Result<T>
  3. 保持向后兼容: 旧函数标记[[deprecated]]
- **风险**: 中 — 大范围API变更
- **评估**: 编译通过, 无deprecated warning(完全迁移后)

### TASK-015: 实现BuildCsvPage/BuildDsPage/GameAlignPage
- **目标**: 完成labeler步骤5/6/7
- **实现步骤**:
  1. BuildCsvPage: 调用TranscriptionPipeline的TextGrid→CSV转换
  2. BuildDsPage: 调用CsvToDsConverter (需解决F0 callback)
  3. GameAlignPage: 调用Game::alignCSV()
- **风险**: 中 — BuildDsPage依赖F0提取(CsvToDsConverter的TODO)
- **评估**: 各步骤可运行并产出正确格式文件

---

## P3 任务

### TASK-017: AudioDecoder实现流式解码

---

## 依赖关系图

```
TASK-009 ──→ 需要TASK-015 (步骤5/6/7实现后向导才完整)

TASK-015 ──→ 依赖F0提取实现 (core CsvToDsConverter TODO)
```

## 工作量估算

| 任务 | 预计人天 | 复杂度 |
|------|----------|--------|
| TASK-005 | 3 | 高 |
| TASK-007 | 3 | 高 |
| TASK-009 | 5 | 高 |
| TASK-010 | 3 | 中 |
| TASK-011 | 1 | 低 |
| TASK-012 | 5 | 高 |
| TASK-015 | 5 | 高 |
