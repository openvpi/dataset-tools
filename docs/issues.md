# DiffSinger Dataset Tools — 未完成 Issue 清单

> 仅列出未完成的可执行 Issue。已完成的 Issue 已归档。

---

### Issue #11: 领域模块单元测试（DsDocument、CsvToDsConverter 等）

**类型**: Tech Debt | **优先级**: P2 | **估计工作量**: XL (3-5d)

**验收标准**:
- [x] DsDocument 测试（加载 .ds、句子解析）
- [ ] CsvToDsConverter 测试（正常转换、格式错误输入）
- [ ] TextGridToCsv 测试
- [x] PitchUtils / F0Curve 测试（数值精度）
- [x] PhNumCalculator 测试
- [x] TranscriptionCsv 测试
- [ ] 测试样本数据放入 `src/tests/data/`

> **进展**: 框架核心已有 11 个单元测试（TestServiceLocator、TestAppSettings、TestAppShellIntegration、TestAsyncTask、TestIDocument、TestIFileIOProviderMock、TestJsonHelper、TestLocalFileIOProvider、TestModelDownloader、TestModelManager、TestResult）。领域层已有 TestDsDocument（17 个测试）、TestPhNumCalculator（11 个测试）、TestTranscriptionCsv（13 个测试）。TestF0Curve 和 TestPitchUtils 已有。CsvToDsConverter 和 TextGridToCsv 需要真实文件样本，待后续补充。

---

### Issue #15: 各框架模块可独立编译

**类型**: Feature | **优先级**: P1 | **估计工作量**: L (1-3d)

**验收标准**:
- [x] 每个框架模块 CMakeLists.txt 可独立构建
- [x] 依赖通过 `find_package` 获取
- [ ] CI 验证脚本分别构建各模块

> **进展**: dsfw-core 和 dsfw-ui-core 已添加 `find_package` guards（`if(NOT TARGET ...)` 模式），确保独立构建时通过 CMake 包配置获取依赖，monorepo 构建时跳过。dstools-types 为 header-only INTERFACE 库，本身无外部依赖，已可独立编译。CI 验证脚本待实现。

---

### Issue #16: 通用框架接口 API 文档

**类型**: Documentation | **优先级**: P2 | **估计工作量**: L (1-3d)

**验收标准**:
- [x] 所有 `I*.h` 接口添加完整 Doxygen 注释
- [x] 配置 Doxyfile 生成 HTML 文档
- [ ] CI 文档生成步骤

> **进展**: T-44 已为全部 23 个框架头文件添加 Doxygen 注释。Doxyfile 已在仓库根目录配置（INPUT 指向 framework headers，GENERATE_HTML=YES）。CI 文档生成步骤待添加（build.yml 中尚无 Doxygen 相关 step）。

---

### Issue #18: dstools-widgets DLL 导出符号审查

**类型**: Tech Debt | **优先级**: P2 | **估计工作量**: M (2-8h)

**验收标准**:
- [x] 确认 12 个 widget 类正确使用导出宏
- [x] 确认内部实现类未被导出
- [ ] 三平台编译验证

> **审查结果**: 审查全部 12 个 widget 类 + 2 个公开 struct（GpuInfo、ViewportState、ShortcutEntry）。发现 `GpuInfo` 缺少 `DSTOOLS_WIDGETS_API` 宏，已修复。其余类/struct 均正确导出，无内部类被误导出。

---

### Issue #19: labeler-interfaces 版本号和兼容性策略

**类型**: Feature | **优先级**: P2 | **估计工作量**: S (< 2h)

---

### Issue #21: CI 矩阵构建验证

**类型**: Feature | **优先级**: P2 | **估计工作量**: L (1-3d)

---

### Issue #27: clang-tidy 和静态分析集成到 CI

**类型**: Feature | **优先级**: P3 | **估计工作量**: M (2-8h)

> **进展**: `.clang-tidy` 配置文件已创建（bugprone/modernize/readability/performance 检查）。CI 集成待完成——需要 CMake configure 生成 `compile_commands.json` 后才能在 CI 中运行 clang-tidy。

---

### Issue #28: TranscriptionPipeline 可测试性改造

**类型**: Refactor | **优先级**: P2 | **估计工作量**: M (2-8h)

> **审查结果 (2026-05-01)**:
>
> #### 当前设计
> - `TranscriptionPipeline::run()` 是一个静态方法，内部串行执行 4 个步骤（TextGrid 提取 → PhNum 计算 → GAME 对齐 → DS 转换）
> - GAME 对齐和 F0 回调通过 `std::function` 注入——这是好的设计，允许外部控制推理行为
>
> #### 可测试性障碍
> 1. ~~**直接依赖文件系统**~~: 已通过 Deps 回调注入解决
> 2. ~~**`PhNumCalculator` 硬编码实例化**~~: 已通过 Deps.loadDictionary 回调解决
> 3. ~~**`CsvToDsConverter::convertFromMemory` 静态调用**~~: 已通过 Deps.convertToDs 回调解决
> 4. **将字符串解析逻辑提取为纯函数**: Step 7 中 phSeq/phDur/phNum 的 split→vector 转换和 notes→row 回写逻辑可提取为无副作用的工具函数，直接单元测试
> 5. ~~**单个巨型静态函数**~~: 已拆分为 4 个独立步骤方法
> 6. ~~**Checkpoint/resume 逻辑与主流程耦合**~~: CSV 存在性检查和 resume 跳转混在 run() 开头，增加了测试路径组合
>
> #### 改进建议
> 1. ~~**引入接口**: 为 `TextGridToCsv`、`PhNumCalculator`、`CsvToDsConverter` 定义轻量接口（或将它们作为回调注入），使测试可提供 mock~~
> 2. ~~**拆分为独立步骤方法**: 将 run() 拆为 `extractTextGrids()`、`calculatePhNum()`、`gameAlign()`、`convertToDs()` 四个方法，每个可独立测试~~
> 3. ~~**文件 I/O 抽象**: 通过注入 `IFileSystem` 接口或将文件操作封装为回调，消除对真实文件系统的依赖~~
> 4. **将字符串解析逻辑提取为纯函数**: Step 7 中 phSeq/phDur/phNum 的 split→vector 转换和 notes→row 回写逻辑可提取为无副作用的工具函数，直接单元测试
> 5. ~~**将 PhNumCalculator 作为构造参数或回调注入**: 类似已有的 `GameAlignCallback` 模式~~

> **进展 (2026-05-01)**: 建议项 1-3 和 5 已实现。Deps 结构体已支持注入 extractTextGrids、loadDictionary、readCsv、writeCsv、convertToDs 回调。Pipeline 已拆分为 4 个独立步骤方法（extractTextGrids、calculatePhNum、gameAlign、convertToDs），每个方法可独立测试。建议项 4（字符串解析纯函数提取）留待后续。

---

### Issue #37: 合并两套 Slicer 实现

**类型**: Refactor | **优先级**: P2 | **估计工作量**: M (2-8h)

**问题**: `audio-util/Slicer` 和 `libs/slicer/Slicer` 实现相同 RMS 切片算法，互不引用。

**验收标准**:
- [ ] 以 audio-util 版本为底层实现
- [ ] libs/slicer 改为薄封装
- [ ] DatasetPipeline 和 DiffSingerLabeler 功能不变

---

### Issue #38: MinLabelPage 业务逻辑提取到 domain 层

**类型**: Refactor | **优先级**: P2 | **估计工作量**: M (2-8h)

**问题**: MinLabelPage.cpp (~560 行) 包含直接文件 I/O、JSON 解析、.lab 写入、目录扫描等业务逻辑。

**验收标准**:
- [ ] 提取 `MinLabelService` 到 domain 层
- [ ] MinLabelPage 仅调用服务接口

---

### Issue #39: God class 拆分

**类型**: Refactor | **优先级**: P3 | **估计工作量**: L (1-3d)

**涉及文件**:
- `PitchLabelerPage.cpp` (781 行)
- `PhonemeLabelerPage.cpp` (630 行)
- `GameModel.cpp` (602 行)
- `PianoRollView.cpp` (579 行)
- `MainWidget.cpp` (GameInfer, 505 行)

---

### Issue #40: 魔法数字提取为命名常量

**类型**: Tech Debt | **优先级**: P3 | **估计工作量**: S (< 2h)

**涉及**: 音频 buffer size `4096`（3 处重复）、窗口尺寸、hop size/采样率范围、Blackman-Harris 窗函数系数。
