# 技术债务

本文档记录项目中已识别的技术债务，按严重程度排序。每项包含位置、问题描述、影响范围和建议修复方案。

---

## TD-001: 推理库 DML/CUDA 初始化代码重复 5 次 [高]

**位置:** game-infer/GameModel.cpp, some-infer/SomeModel.cpp, rmvpe-infer/RmvpeModel.cpp, hubert-infer/HfaModel.cpp, common/OnnxEnv.cpp

**描述:** 约 130 行 `initDirectML()` / `initCUDA()` 代码在 5 个文件中完全重复。dstools-infer-common 模块已提供 `OnnxEnv` 单例，但所有推理库都忽略它，各自创建独立的 `Ort::Env` 实例。

**影响:**
- 修改 EP 初始化逻辑需改 5 处，遗漏任何一处都会导致行为不一致
- 每个推理库创建独立 `Ort::Env` 实例，浪费资源

**修复:** 所有推理库统一使用 `OnnxEnv::env()` 和 `OnnxEnv::createSessionOptions()`，删除各自的初始化代码。

---

## TD-002: FunAsr 模块代码质量极差 [高]

**位置:** src/infer/FunAsr/src/ (全部文件)

**描述:** 典型的 C 风格代码，问题包括：
- `malloc`/`free`、raw `new`/`delete`，无 RAII
- 硬编码 magic number (8404, 512, 400, 160, 67)
- 无错误处理
- FFTW 非线程安全使用

**影响:** 内存泄漏、崩溃风险、无法维护。

**修复:** 重写为现代 C++，使用 smart pointer，将 magic number 提取为 config 参数。

---

## TD-004: CMake GLOB_RECURSE 收集源文件 [中]

**位置:** 所有 apps 和 infer 库的 CMakeLists.txt

**描述:** 使用 `file(GLOB_RECURSE)` 收集源文件。CMake 官方文档明确警告：新增文件不会自动检测，需手动重新运行 cmake。

**影响:** 开发者新增 `.cpp` 文件后忘记重新 cmake，链接时报找不到符号，排查耗时。

---

## TD-005: 错误报告风格不统一 [中]

**位置:** CsvToDsConverter(`std::string`), TranscriptionPipeline(`QString`), PhNumCalculator(`QString`), DsItemManager(`std::string`)

**描述:** core 层混用 `std::string &error` 和 `QString &error` 两种错误输出方式。同一 pipeline 链中需要反复转换。

**修复:** 统一为一种。建议全部使用 `QString`（Qt 项目），或引入 `Result<T, E>` 类型。

---

## TD-007: AudioDecoder 全量解码到内存 [低]

**位置:** src/audio/src/AudioDecoder.cpp

**描述:** `decodeAll()` 将整个音频文件解码到内存。10 分钟 44.1kHz 立体声 float 约 210MB。

**影响:** 大文件时内存占用极高。

**修复:** 实现流式解码/分块读取。

---

## TD-008: 路径类型不统一 [低]

**描述:** `DsItemManager` 用 `std::filesystem::path`，其他类用 `QString`。调用边界需要反复转换。

**修复:** 统一为一种，或提供全局转换工具函数。
