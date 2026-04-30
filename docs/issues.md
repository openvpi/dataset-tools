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
- [ ] 确认 12 个 widget 类正确使用导出宏
- [ ] 确认内部实现类未被导出
- [ ] 三平台编译验证

---

### Issue #19: labeler-interfaces 版本号和兼容性策略

**类型**: Feature | **优先级**: P2 | **估计工作量**: S (< 2h)

---

### Issue #20: dstools-infer-common 与框架层解耦

**类型**: Refactor | **优先级**: P2 | **估计工作量**: M (2-8h)

---

### Issue #21: CI 矩阵构建验证

**类型**: Feature | **优先级**: P2 | **估计工作量**: L (1-3d)

---

### Issue #22: ModelDownloader / ModelManager 迁移到 domain 层

**类型**: Refactor | **优先级**: P2 | **估计工作量**: S (< 2h)

> **状态**: ModelManager 和 ModelDownloader 仍在 `src/framework/core/`（含 IModelDownloader 接口）。Roadmap T-1.1 涵盖定义 IModelManager 接口并将具体实现迁移到 domain 层，此 Issue 将随 T-1.1 一并解决。

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

---

### Issue #27: clang-tidy 和静态分析集成到 CI

**类型**: Feature | **优先级**: P3 | **估计工作量**: M (2-8h)

---

### Issue #28: TranscriptionPipeline 可测试性改造

**类型**: Refactor | **优先级**: P2 | **估计工作量**: M (2-8h)
