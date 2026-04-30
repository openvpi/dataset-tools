# DiffSinger Dataset Tools — 未完成 Issue 清单

> 仅列出未完成的可执行 Issue。已完成的 Issue 已归档。

---

### Issue #7: 修复 pipeline-pages 使用 `../slicer/` 相对路径引用源文件

**类型**: Tech Debt | **优先级**: P1 | **估计工作量**: L (1-3d)

`pipeline-pages` 通过相对路径直接引用 slicer/lyricfa/hubertfa 源文件。应将三者构建为独立静态库。

**验收标准**:
- [x] slicer、lyricfa、hubertfa 各自有独立 CMakeLists.txt，构建为静态库
- [x] pipeline-pages 通过 `target_link_libraries` 链接
- [x] DatasetPipeline 和 DiffSingerLabeler 编译运行正常

> **已完成**: slicer-lib、lyricfa-lib、hubertfa-lib 已作为独立静态库存在于 src/libs/，pipeline-pages 已通过 target_link_libraries 链接。

---

### Issue #8: 重构 WorkThread.cpp，拆分业务逻辑

**类型**: Refactor | **优先级**: P1 | **估计工作量**: L (1-3d)

> 注: SliceJob 已从 WorkThread 提取，WorkThread 已降为薄壳。此 Issue 可视为大部分完成，剩余为进一步细化。

**验收标准**:
- [ ] 音频切片逻辑提取为独立类
- [ ] Marker 文件读写提取为独立类
- [ ] 构造函数参数使用配置结构体

---

### Issue #10: IDocument / IFileIOProvider 接口单元测试

**类型**: Tech Debt | **优先级**: P1 | **估计工作量**: L (1-3d)

> TestLocalFileIOProvider 已编写。Mock 实现和错误路径测试待做。

**验收标准**:
- [ ] IDocument mock 实现 + 文档生命周期测试
- [ ] IFileIOProvider mock 实现 + 文件操作契约测试
- [ ] 错误路径测试（文件不存在、权限不足等）

---

### Issue #11: 领域模块单元测试（DsDocument、CsvToDsConverter 等）

**类型**: Tech Debt | **优先级**: P2 | **估计工作量**: XL (3-5d)

**验收标准**:
- [ ] DsDocument 测试（加载 .ds、句子解析）
- [ ] CsvToDsConverter 测试（正常转换、格式错误输入）
- [ ] TextGridToCsv 测试
- [ ] PitchUtils / F0Curve 测试（数值精度）
- [ ] 测试样本数据放入 `src/tests/data/`

---

### Issue #12: Bug — BaseFileListPanel 中 file.open() 缺少错误处理

**类型**: Bug | **优先级**: P1 | **估计工作量**: S (< 2h)

**验收标准**:
- [ ] 所有 `file.open()` 添加错误检查
- [ ] 失败时向用户显示明确错误信息

---

### Issue #14: dstools-types CMake 包导出配置

**类型**: Feature | **优先级**: P1 | **估计工作量**: M (2-8h)

**验收标准**:
- [x] `install(TARGETS dstools-types EXPORT ...)` 规则
- [x] 外部项目可 `find_package(dstools-types CONFIG REQUIRED)`
- [x] header-only 属性正确导出

> **已完成**: CMake export/install 规则已实现，支持 find_package(dstools-types CONFIG REQUIRED)。

---

### Issue #15: 各框架模块可独立编译

**类型**: Feature | **优先级**: P1 | **估计工作量**: L (1-3d)

**验收标准**:
- [ ] 每个框架模块 CMakeLists.txt 可独立构建
- [ ] 依赖通过 `find_package` 获取
- [ ] CI 验证脚本分别构建各模块

---

### Issue #16: 通用框架接口 API 文档

**类型**: Documentation | **优先级**: P2 | **估计工作量**: L (1-3d)

**验收标准**:
- [ ] 所有 `I*.h` 接口添加完整 Doxygen 注释
- [ ] 配置 Doxyfile 生成 HTML 文档
- [ ] CI 文档生成步骤

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

### Issue #20: dstools-infer-common 与框架层解耦

**类型**: Refactor | **优先级**: P2 | **估计工作量**: M (2-8h)

> **已完成**: T-0.3 已移除 Qt 依赖，dstools-infer-common 现仅依赖 dstools-types（header-only）+ onnxruntime。

---

### Issue #21: CI 矩阵构建验证

**类型**: Feature | **优先级**: P2 | **估计工作量**: L (1-3d)

---

### Issue #22: ModelDownloader / ModelManager 迁移到 domain 层

**类型**: Refactor | **优先级**: P2 | **估计工作量**: S (< 2h)

> **部分完成**: 具体实现仍在 dsfw-core，完整迁移见 T-1.1。

---

### Issue #23: QualityMetrics 实现迁移到 domain 层

**类型**: Refactor | **优先级**: P2 | **估计工作量**: S (< 2h)

> **已完成**: QualityMetrics.cpp / QualityMetrics.h 已在 `src/domain/`，CMakeLists.txt 已包含。

---

### Issue #24: PinyinG2PProvider 实现迁移到 domain 层

**类型**: Refactor | **优先级**: P2 | **估计工作量**: S (< 2h)

> **已完成**: PinyinG2PProvider.cpp / PinyinG2PProvider.h 已在 `src/domain/`，CMakeLists.txt 已包含。

---

### Issue #25: ExportFormats.h header-only 实现移到 .cpp

**类型**: Tech Debt | **优先级**: P3 | **估计工作量**: S (< 2h)

> **已完成**: ExportFormats.h 仅包含类声明，实现已在 ExportFormats.cpp 中。

---

### Issue #26: DsDocumentAdapter / DsItemManager 职责边界审查

**类型**: Tech Debt | **优先级**: P3 | **估计工作量**: M (2-8h)

> **审查结果 (2026-05-01)**:
>
> #### DsDocumentAdapter 职责
> - **角色**: 适配器模式——将 `DsDocument`（DS 文件的内存表示）适配为框架层的 `IDocument` 接口
> - **具体职责**:
>   - 实现 `IDocument` 的 Identity（filePath、format、formatDisplayName）、Lifecycle（load/save/saveAs/close）、State（isModified/setModified/info）接口
>   - 提供 Content access 方法：`entryCount()`（委托 sentenceCount）和 `durationSec()`（遍历句子计算总时长）
>   - 管理自身的 `m_filePath` 和 `m_modified` 状态（DsDocument 本身不跟踪修改标志）
>   - 通过 `inner()` / `innerShared()` 暴露底层 DsDocument 供上层直接操作句子数据
>   - 在 load/save 中做 QString ↔ std::string 错误类型转换（IDocument 接口用 std::string，DsDocument 用 QString）
>
> #### DsItemManager 职责
> - **角色**: 管理 `.dsitem` JSON 文件，跟踪 DiffSinger 数据集处理流水线中每个文件在每个步骤的处理状态
> - **具体职责**:
>   - 文件路径计算：`projectRoot/dstemp/<step>/<baseName>.dsitem`
>   - CRUD：load/save DsItemRecord（JSON 序列化/反序列化）
>   - 增量处理判断：`needsProcessing()` 比较源文件修改时间与 .dsitem 时间戳
>   - 批量查询：`summarizeStep()` 统计某步骤的 completed/failed/pending 数量
>   - 状态更新：`markCompleted()` / `markFailed()`
>   - 枚举映射：PipelineStep ↔ string、ItemStatus ↔ string
>
> #### DsDocument 职责（参考）
> - DS 文件（JSON 数组格式）的纯数据模型：加载、保存、句子访问、字段类型处理
> - 不涉及修改状态跟踪或框架接口
>
> #### 职责边界评估
> - **无重叠**: DsDocumentAdapter 和 DsItemManager 职责完全不同——前者是 IDocument 接口适配器，后者是流水线状态追踪器。两者在不同场景使用，不存在交叉
> - **DsDocumentAdapter 的 `durationSec()` 包含业务逻辑**: 遍历句子并解析 `ph_dur` 字段来计算时长，这属于 DS 文件语义理解，可考虑下沉到 DsDocument 本身
> - **DsItemManager 的 `DsDocument::toFsPath` 依赖**: `needsProcessing()` 调用了 `DsDocument::toFsPath()`，形成对 DsDocument 的编译依赖。该静态方法实质上是通用路径转换工具，可考虑提取到独立工具类
> - **命名可能引起混淆**: 两者均以 `Ds` 前缀命名且位于同一命名空间，但管理完全不同的概念（DS 文件内容 vs 流水线处理状态）。当前代码中职责清晰，不需要改名
>
> #### 改进建议（不影响当前功能）
> 1. 将 `DsDocumentAdapter::durationSec()` 中的时长计算逻辑移至 `DsDocument`，让 Adapter 纯粹委托
> 2. 将 `DsDocument::toFsPath()` 提取为独立工具函数（如 `PathUtils::toFsPath`），消除 DsItemManager 对 DsDocument 的不必要依赖
> 3. 考虑为 DsDocumentAdapter 的 load/save 迁移到 Result-based API（DsDocument 已有 `loadFile()` / `saveFile()` 返回 Result，但 Adapter 仍使用已弃用的 QString error 版本）

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
> 1. **直接依赖文件系统**: `TextGridToCsv::extractDirectory()` 直接读取 `opts.textGridDir` 目录中的 TextGrid 文件；`QFileInfo::exists(opts.csvPath)` 检查文件存在；`TranscriptionCsv::read/write` 直接操作文件——测试必须准备真实文件
> 2. **`PhNumCalculator` 硬编码实例化**: 在 run() 内部直接 `PhNumCalculator calc; calc.loadDictionary(opts.dictPath, ...)`，无法注入 mock。字典文件也必须存在
> 3. **`CsvToDsConverter::convertFromMemory` 静态调用**: 无法替换为 mock，且该函数内部可能涉及文件 I/O（写 .ds 文件到 outputDir）
> 4. **`TextGridToCsv::extractDirectory` 静态调用**: 同上，无法在测试中替换
> 5. **单个巨型静态函数**: 150 行逻辑全在一个函数中，无法单独测试某个步骤（如只测 Step 7 的字符串解析逻辑）
> 6. **Checkpoint/resume 逻辑与主流程耦合**: CSV 存在性检查和 resume 跳转混在 run() 开头，增加了测试路径组合
>
> #### 改进建议
> 1. **引入接口**: 为 `TextGridToCsv`、`PhNumCalculator`、`CsvToDsConverter` 定义轻量接口（或将它们作为回调注入），使测试可提供 mock
> 2. **拆分为独立步骤方法**: 将 run() 拆为 `extractTextGrids()`、`calculatePhNum()`、`gameAlign()`、`convertToDs()` 四个方法，每个可独立测试
> 3. **文件 I/O 抽象**: 通过注入 `IFileSystem` 接口或将文件操作封装为回调，消除对真实文件系统的依赖
> 4. **将字符串解析逻辑提取为纯函数**: Step 7 中 phSeq/phDur/phNum 的 split→vector 转换和 notes→row 回写逻辑可提取为无副作用的工具函数，直接单元测试
> 5. **将 PhNumCalculator 作为构造参数或回调注入**: 类似已有的 `GameAlignCallback` 模式
