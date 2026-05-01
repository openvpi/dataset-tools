# DiffSinger Dataset Tools — 架构分析报告

> 最后更新: 2026-04-30

---

## 1. 项目概况

| 维度 | 选型 |
|------|------|
| 语言 | C++17 |
| GUI | Qt 6.8+ (Core, Gui, Widgets, Svg, Network) |
| 推理 | ONNX Runtime (DirectML / CUDA / CPU) |
| 构建 | CMake >= 3.17 + vcpkg |
| 音频 | FFmpeg + SDL2 + SndFile + mpg123 + soxr |
| 平台 | Windows 10/11 (主), macOS 11+, Linux |

6 个应用：DatasetPipeline, MinLabel, PhonemeLabeler, PitchLabeler, GameInfer, DiffSingerLabeler。

## 2. 当前模块依赖图

```
┌──────────────────────────────────────────────────────────────────────┐
│                       应用层 (6 个可执行文件)                         │
├──────────────────────────────────────────────────────────────────────┤
│  页面库 (STATIC): minlabel-page, phonemelabeler-page,               │
│  pitchlabeler-page, pipeline-pages, labeler-interfaces (INTERFACE)  │
├──────────────────────────────────────────────────────────────────────┤
│  dstools-widgets (SHARED DLL)                                        │
├─────────────────────────┬────────────────────────────────────────────┤
│  dsfw-ui-core (STATIC)  │  dstools-audio (STATIC)                    │
├─────────────────────────┤                                            │
│  dsfw-core (STATIC)     │                                            │
├────────────┬────────────┤                                            │
│dstools-    │dstools-    │                                            │
│domain      │types       │                                            │
│(STATIC)    │(INTERFACE) │                                            │
├────────────┴────────────┴────────────────────────────────────────────┤
│  推理层: dstools-infer-common, game/hubert/rmvpe-infer, audio-util   │
└──────────────────────────────────────────────────────────────────────┘
```

## 3. 遗留缺陷

| 缺陷 | 严重性 | 说明 |
|------|--------|------|
| dsfw-core 依赖 Qt::Gui | 低 | 纯逻辑代码无法避免引入 Qt Gui 模块，实际影响小 |
| pipeline-pages 相对路径源码引用 | 低 | `../slicer/` 等路径引用源码，可改为独立静态库 |

## 4. 技术债

| 项目 | 严重性 | 状态 |
|------|--------|------|
| WorkThread.cpp 业务逻辑分离 | 中 | SliceJob 已提取，WorkThread 已降为薄壳 |
| 测试覆盖 | 中 | 8 个 dsfw 核心类测试 + 1 集成测试已有；领域模块零测试 |
| 文件打开缺少错误分支 | 低 | 部分修复 |

## 5. 剩余任务

### 高优先级

| ID | 任务 | 说明 |
|----|------|------|
| TEST-001 | 领域纯逻辑类单元测试 | PitchUtils, F0Curve, CsvToDsConverter, TextGridToCsv, PitchProcessor |
| TEST-002 | 领域服务类单元测试 | DsDocument, TranscriptionPipeline |

### 低优先级

| ID | 任务 | 说明 |
|----|------|------|
| ARCH-005 | pipeline 子模块独立库化 | slicer/lyricfa/hubertfa 各自编译为 STATIC lib |
| ARCH-006 | dstools-types 补充共享类型 | 下沉更多跨模块 enum/POD |
| BUG-002 | 文件打开失败添加错误处理 | 多个 FileListPanel 文件 |
