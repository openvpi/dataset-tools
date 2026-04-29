# PRD 差距分析

## 概述

本文档对比产品声明功能与实际实现状态, 识别功能缺口.

## GAP-001: DiffSingerLabeler 9步流程仅完成3步 [严重]

- README声称: "Unified 9-step dataset labeling wizard integrating Slice, ASR, Label, Align, Phone, CSV/MIDI/DS build, and Pitch"
- 实际状态:
  - ✅ 步骤2 (Label/MinLabel): 已集成
  - ✅ 步骤4 (Phone/PhonemeLabeler): 已集成
  - ✅ 步骤8 (Pitch/PitchLabeler): 已集成
  - ❌ 步骤0 (Slice): 仅QLabel占位符
  - ❌ 步骤1 (ASR): 仅QLabel占位符
  - ❌ 步骤3 (Align): 仅QLabel占位符
  - ❌ 步骤5 (Build CSV): UI存在但"Not Implemented"
  - ❌ 步骤6 (Build DS): UI存在但"Not Implemented"
  - ❌ 步骤7 (GAME Align): UI存在但"Not Implemented"
- 影响: 用户无法通过单一向导完成全流程, 需手动在多个独立应用间切换

## GAP-002: 跨平台声明与实际支持不匹配 [严重]

- README声称: "Microsoft Windows (10~11), Apple macOS (11+), Linux (Ubuntu)"
- 实际CI: 仅Windows构建和测试
- 实际代码问题:
  - Linux无install/deploy逻辑 (src/CMakeLists.txt 无else分支)
  - MinLabel字典文件仅Windows复制
  - FunAsr CMakeLists.txt硬编码macOS Homebrew路径
  - setup-onnxruntime.cmake不支持Linux ARM64
- 影响: macOS/Linux用户构建可能失败或运行时功能缺失

## GAP-003: PitchLabeler声称Undo/Redo但未实现 [中等]

- README声称: "undo/redo" 功能
- 实际: onUndo()/onRedo()方法体为空, TODO注释
- 影响: 用户编辑F0曲线后无法撤销, 误操作不可恢复

## GAP-004: 模型自动下载未实现 [中等]

- 接口设计: IModelDownloader已定义download()接口
- 实际: StubModelDownloader返回错误
- 当前流程: 用户需手动从GitHub Releases下载模型文件并放到正确目录
- 影响: 新用户上手门槛高, 容易路径配错

## GAP-005: 质量评估系统未实现 [低]

- 接口设计: IQualityMetrics定义了evaluate()接口
- 实际: StubQualityMetrics返回空
- 预期功能: 数据集SNR评分、对齐置信度、音素覆盖率统计
- 影响: 用户无法评估数据集质量

## GAP-006: 插件系统未实现 [低]

- 接口设计: IStepPlugin定义了完整的插件生命周期
- 实际: StubStepPlugin, TranscriptionPipeline硬编码步骤
- 预期功能: 第三方可扩展处理步骤
- 影响: 无法扩展管线功能

## GAP-007: Pipeline Slicer格式支持不完整 [中等]

- 预期: 项目依赖FFmpeg, 应支持多种音频格式
- 实际: SlicerPage只接受.wav文件 (硬编码后缀检查)
- 影响: 用户需先手动转换格式

## GAP-008: 测试覆盖为零 [中等]

- CI中BUILD_TESTS=OFF, src/tests/CMakeLists.txt为空文件
- 虽然infer子库有各自tests (TestGame, TestSome等), 但:
  - CI从不执行
  - 无单元测试框架集成
  - 无UI测试
  - 无集成测试
- 影响: 代码变更无法回归验证

## GAP-009: FunAsr无GPU加速 [低]

- 其他推理库: 全部支持DirectML + CUDA
- FunAsr: 仅CPU
- 影响: ASR步骤是唯一无法GPU加速的推理步骤
