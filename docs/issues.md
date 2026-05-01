# DiffSinger Dataset Tools — Issue 清单

> 最后更新：2026-05-01

---

## 开放 Issue

| Issue # | 标题 | 优先级 | 状态 | 路线图 |
|---------|------|--------|------|--------|
| #11 | 领域模块单元测试 | P2 | ⏳ 部分完成 | B.1 |
| #15 | 框架模块独立编译 | P2 | ⏳ CI 未验证 | D.1 |
| #16 | API 文档 CI | P3 | ⏳ CI 未完成 | D.2 |
| #28 | TranscriptionPipeline 可测试性 | P2 | ⏳ 障碍已清除，待写测试 | B.1 |
| #39 | 大文件拆分 | P3 | 📋 按需 | C.1 |
| #40 | 魔法数字 | P3 | 📋 按需 | C.2 |

**已关闭**: #18 (DLL 导出审查 — 已修复 GpuInfo，CI 矩阵构建已覆盖三平台验证), #19 (labeler-interfaces — 无外部消费者，暂不需要版本策略), #21 (CI 矩阵), #27 (clang-tidy), #37 (Slicer), #38 (MinLabel)

---

### #11: 领域模块单元测试

已完成 6 个测试模块 (TestDsDocument, TestF0Curve, TestPitchUtils, TestPhNumCalculator, TestTranscriptionCsv, TestStringUtils, TestMinLabelService)。

待完成: CsvToDsConverter, TextGridToCsv, PitchProcessor, TranscriptionPipeline 集成测试。

### #15: 框架模块独立编译

dsfw-core / dsfw-ui-core 已有 `find_package` guards。缺 `.github/workflows/verify-modules.yml` CI 验证。

### #16: API 文档 CI

23 个框架头文件已有 Doxygen 注释，Doxyfile 已配置。缺 CI 自动生成 + GitHub Pages 发布。

### #28: TranscriptionPipeline 可测试性

Deps 注入、步骤拆分、StringUtils 提取均已完成。待编写集成测试用例。

### #39: 大文件拆分

PitchLabelerPage (781行)、PhonemeLabelerPage (630行) 有维护痛点时拆分。

### #40: 魔法数字

`4096` buffer size 3 处重复，优先提取。其余按需。
