# DiffSinger Dataset Tools 改进方向

---

## 1. 近期改进

### 1.1 领域模块单元测试

框架核心类已有 11 个测试。领域模块已有 5 个测试（TestDsDocument、TestF0Curve、TestPhNumCalculator、TestPitchUtils、TestTranscriptionCsv），共 16 个测试文件。

| 模块 | 测试重点 | 状态 |
|------|---------|------|
| DsDocument | 加载/保存 .ds 文件、句子解析 | ✅ TestDsDocument (17 tests) |
| CsvToDsConverter | 正常转换、格式错误输入 | 待测 |
| TextGridToCsv | TextGrid 解析与转换 | 待测 |
| PitchUtils / F0Curve | 数值计算精度 | ✅ TestPitchUtils, TestF0Curve |
| PhNumCalculator | 音素数量计算 | ✅ TestPhNumCalculator (11 tests) |
| TranscriptionCsv | CSV 转录 | ✅ TestTranscriptionCsv (13 tests) |
| PitchProcessor | 纯逻辑 DSP/编辑算法 | 待测 |

### 1.2 pipeline 子模块独立库化 ✅ 已完成

slicer-lib、lyricfa、hubertfa 已编译为独立静态库，pipeline pages 通过 target_link_libraries 链接。

### 1.3 DocumentFormat / ModelType 枚举泛化 ✅ 已完成

`DocumentFormat` 已替换为 `DocumentFormatId` 注册模式，`ModelType` 已替换为 `ModelTypeId` 注册模式。框架层不再包含 DiffSinger 特定枚举值。

---

## 2. 中期改进

### 2.1 框架独立仓库

已决定保留单仓库模式（见 ADR-8）。框架通过 find_package(dsfw) 支持外部消费。

### 2.2 API 稳定化 + 语义版本控制

- 标记公开头文件 `DSTOOLS_EXPORT` 宏
- 引入语义版本号
- 建立 CHANGELOG.md

### 2.3 AppShell 完善

- 标签页式导航（可选）、页面分组
- 页面懒加载

---

## 3. 长期改进

### 3.1 插件热加载

基于 `QPluginLoader` 实现运行时加载/卸载，定义 `IPlugin` 接口标准。

### 3.2 跨平台 CI/CD

GitHub Actions 矩阵构建（Windows/Linux/macOS），自动运行测试和打包。

### 3.3 Doxygen 文档生成

Doxyfile 已配置，23 个框架头文件已有 Doxygen 注释。仅缺 CI 自动构建步骤。

### 3.4 示例项目

创建最小可用的非 DiffSinger 示例 app，放入 `examples/`。

### 3.5 vcpkg / Conan 包发布

编写 vcpkg port 文件或 Conan recipe，提交到 registry。

---

## 4. 代码质量

### 4.1 clang-tidy 集成

.clang-tidy 配置已创建。CI 集成待完成。

### 4.2 API Review Checklist

每次新增/修改公开 API 时检查：命名一致性、Doxygen 注释、参数类型、Result<T> 错误处理、线程安全性、单元测试、二进制兼容性。
