# 架构缺陷与技术债务清单

> 最后更新：2026-05-01

---

## Bug — 全部已修复 ✅

BUG-03 (languageMap 数据竞争)、BUG-04 (use-after-free)、BUG-05 (double-free)、BUG-06 (未知音素 crash)、BUG-07 (throw 穿透 bool)、BUG-08 (死代码宏)、BUG-09 (缺导出宏)

## 架构债 — 全部已修复 ✅

ARCH-07~09, ARCH-11~13, AD-05~06 均已修复。ARCH-10 (MinLabelPage) 已提取 MinLabelService，442 行属正常范围。

---

## 技术债

| 编号 | 描述 | 严重性 | 路线图 |
|------|------|--------|--------|
| TD-01 | ServiceLocator 使用 void* 存储，类型不匹配时 UB | 中 | A.1 |
| TD-02 | IAsrService 缺 loadModel/isModelLoaded/unloadModel 虚方法，与其他三个服务接口不一致 | 中 | A.2 |
| TD-03 | 4 个领域模块缺单元测试 (CsvToDsConverter, TextGridToCsv, PitchProcessor, TranscriptionPipeline) | 中 | B.1 |
| TD-04 | 部分 `file.open()` 缺错误分支 | 低 | B.3 |
| TD-05 | PitchLabelerPage (781行), PhonemeLabelerPage (630行) 职责过多 | 低 | C.1 |
| TD-06 | `4096` buffer size 3 处重复 | 低 | C.2 |
| TD-07 | 5 处 TODO/FIXME | 低 | B.2 |
| TD-08 | IInferenceEngine::load() 非纯虚，子类可意外不实现 | 低 | A.2 |
| TD-09 | CrashHandler 仅 1/6 app 启用 | 低 | A.3 |
