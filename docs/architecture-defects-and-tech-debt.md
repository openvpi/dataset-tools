# 架构缺陷与技术债务清单

> 最后更新：2026-05-01

---

## 技术债

| 编号 | 描述 | 严重性 | 路线图 |
|------|------|--------|--------|
| TD-03 | 4 个领域模块缺单元测试 (CsvToDsConverter, TextGridToCsv, PitchProcessor, TranscriptionPipeline) | 中 | B.1 |
| TD-04 | 部分 `file.open()` 缺错误分支 | 低 | B.3 |
| TD-05 | PitchLabelerPage (781行), PhonemeLabelerPage (630行) 职责过多 | 低 | C.1 |
| TD-06 | `4096` buffer size 3 处重复 | 低 | C.2 |
| TD-07 | 5 处 TODO/FIXME | 低 | B.2 |
