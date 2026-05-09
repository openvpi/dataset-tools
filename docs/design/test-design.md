# Dataset Tools 测试设计文档

**版本**：1.0  
**日期**：2026-05-06  
**关联规范**：[conventions-and-standards.md](../guides/conventions-and-standards.md)

---

## 1. 目标与原则

### 1.1 目标

建立分层、可维护的自动化测试体系，覆盖以下维度：

- **正确性**：基础类型、框架核心、领域逻辑、流水线各环节的正确行为
- **错误路径**：配置错误、文件缺失、模型加载失败等异常情况
- **格式兼容**：TextGrid、CSV、.ds、.lab 等外部格式的导入导出
- **边界条件**：空输入、极值、格式异常等边界情况

### 1.2 原则

| 原则 | 说明 |
|------|------|
| 分模块组织 | 每个测试目标对应一个独立子目录，便于 review 和选择性运行 |
| 自包含 | 测试使用的 fixture 数据随测试代码存放，不依赖外部绝对路径 |
| 有断言 | 每个测试用例必须有明确的 pass/fail 判定，禁止 print-only |
| CTest 集成 | 所有测试通过 `add_test()` 注册，可用 `ctest --output-on-failure` 统一运行 |
| 无外部依赖 | 单元测试不依赖 ONNX Runtime 等外部库；推理测试可选择性跳过 |

### 1.3 测试框架

使用 Qt Test（`QTest`）：
- 项目已依赖 Qt 6，无需引入新依赖
- 支持 `QCOMPARE`、`QVERIFY`、数据驱动测试
- 与 CTest 天然集成
- `QBENCHMARK` 可替代手写性能计时

---

## 2. 测试层次

### 2.1 三层测试

```
L1  单元测试        纯逻辑验证，不加载模型或资源，不需要 QApplication
L2  集成测试        需要 QApplication/GUI，测试组件交互
L3  推理测试        需要 ONNX 模型文件，测试推理正确性
```

### 2.2 CMake 控制

| 层级 | 控制选项 | 默认 | 说明 |
|------|---------|------|------|
| L1 Unit | `BUILD_TESTS` | ON | 纯逻辑测试，无外部依赖 |
| L2 Integration | `BUILD_INTEGRATION_TESTS` | OFF | 需要 QApplication / GUI |
| L3 Infer | `*_BUILD_TESTS` | OFF | 需要 ONNX 模型文件 |

### 2.3 当前测试目录

```
src/tests/
├── framework/                    # L1: dsfw 核心类单元测试
│   ├── TestTimePos.cpp
│   ├── TestResult.cpp
│   ├── TestJsonHelper.cpp
│   ├── TestAppSettings.cpp
│   ├── TestServiceLocator.cpp
│   ├── TestAsyncTask.cpp
│   ├── TestLocalFileIOProvider.cpp
│   ├── TestModelManager.cpp
│   ├── TestModelDownloader.cpp
│   ├── TestF0Curve.cpp
│   ├── TestCurveTools.cpp
│   └── ...                      # 共 21 个测试
├── CMakeLists.txt                # 推理库测试注册
```

---

## 3. L1 单元测试

### 3.1 基础类型 (dstools-types)

| 测试文件 | 覆盖内容 |
|---------|---------|
| `TestTimePos` | TimePos 算术、比较、μs↔秒转换 |
| `TestResult` | `Result<T>` 值/错误构造、`operator bool`、`value_or`、`Result<void>` |

#### TestResult 用例矩阵

| 用例 | 验证内容 |
|------|---------|
| `valueConstruct` | `Result<int>(42)` → `ok()`, `value() == 42` |
| `errorConstruct` | `Result<int>::Error("fail")` → `!ok()`, `error() == "fail"` |
| `implicitConversion` | `return 42;` 隐式构造 `Result<int>` |
| `voidOk` | `Ok()` → `ok() == true` |
| `voidError` | `Err("msg")` → `!ok()`, `error() == "msg"` |
| `valueOr` | 有值返值，无值返 default |
| `arrowOperator` | `result->member` 访问 |
| `derefOperator` | `*result` 解引用 |

### 3.2 框架核心 (dsfw-core)

| 测试文件 | 覆盖内容 |
|---------|---------|
| `TestJsonHelper` | JSON 读写、类型转换、错误处理 |
| `TestAppSettings` | 设置读写、默认值、类型安全 |
| `TestServiceLocator` | 注册、获取、替换、线程安全 |
| `TestAsyncTask` | 异步执行、取消、回调 |
| `TestLocalFileIOProvider` | 文件读写、目录创建、错误路径 |
| `TestModelManager` | 模型注册、查找、生命周期 |
| `TestModelDownloader` | 下载逻辑、错误处理 |

### 3.3 领域逻辑 (dstools-domain)

| 测试文件 | 覆盖内容 |
|---------|---------|
| `TestF0Curve` | F0 曲线操作、插值、mHz↔Hz 转换 |
| `TestCurveTools` | 曲线平滑、裁剪、合并 |

### 3.4 规划中的测试

| 模块 | 优先级 | 覆盖内容 |
|------|--------|---------|
| `TestPipelineContext` | 高 | Context 序列化/反序列化、层数据读写、状态管理 |
| `TestFormatAdapters` | 高 | TextGrid/CSV/.ds/.lab 导入导出、边界条件、MDS 兼容 |
| `TestPipelineRunner` | 中 | 步骤执行、丢弃传播、错误恢复、快照 |
| `TestTaskProcessorRegistry` | 中 | 注册、I/O 一致性校验、处理器替换 |
| `TestDsProject` | 中 | .dsproj 读写、版本迁移、路径解析 |
| `TestDsDocument` | 中 | .dstext 读写、层数据完整性 |

---

## 4. L2 集成测试

需要 `BUILD_INTEGRATION_TESTS=ON`。

| 测试 | 覆盖内容 |
|------|---------|
| `TestAppShellIntegration` | AppShell 页面切换、菜单/快捷键跟随、生命周期 |
| `dsfw-widgets-test` | 通用 GUI 组件渲染和交互 |

### 规划中

| 测试 | 覆盖内容 |
|------|---------|
| `TestAudioVisualizerContainer` | 视口缩放、滚动、子图布局、边界线交互 |
| `TestPhonemeEditor` | 音素边界拖动、层级选择、clamp 约束 |
| `TestPitchEditor` | F0 曲线绘制、MIDI 叠加显示 |

---

## 5. L3 推理测试

需要对应 `*_BUILD_TESTS=ON` 和模型文件。

| 测试 | 所需模型 | 覆盖内容 |
|------|---------|---------|
| `TestGame` | GAME 模型 | Audio→MIDI 转录正确性 |
| `TestRmvpe` | RMVPE 模型 | F0 提取正确性 |
| `TestAudioUtil` | 无 | 重采样、格式转换、读写 |

---

## 6. Mock 与 Fixture 规范

### 6.1 Fixture 数据

- 测试用 fixture 数据（JSON、TextGrid、.lab、WAV 片段）存放在测试目录下的 `fixtures/` 子目录
- 通过 CMake `target_compile_definitions` 注入路径：

```cmake
target_compile_definitions(${TEST_TARGET} PRIVATE
    TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)
```

### 6.2 路径参数化

消除所有硬编码绝对路径：

```cpp
// 使用编译时定义
const auto fixtureDir = QString(TEST_FIXTURE_DIR);
const auto testFile = fixtureDir + "/sample.textgrid";
```

### 6.3 Mock 服务

对框架接口使用 Stub 实现：

```cpp
class StubG2PProvider : public IG2PProvider { ... };
class StubModelDownloader : public IModelDownloader { ... };
```

框架已提供 `StubG2PProvider`、`StubExportFormat`、`StubQualityMetrics`、`StubModelDownloader` 等。

---

## 7. Review 指南

### 7.1 测试 Review 检查清单

- [ ] 每个测试用例是否有明确的断言？
- [ ] 测试用例名是否清晰描述场景和预期？
- [ ] 错误路径测试是否验证了错误消息的关键内容？
- [ ] fixture 数据是否自包含？
- [ ] 是否有遗漏的边界条件？
- [ ] 异步测试是否使用了 `QSignalSpy` 或 `QTRY_VERIFY` 等待？

### 7.2 测试命名

```cpp
void TestResult::valueConstruct()      // 描述性命名
void TestResult::errorPropagation()    // 描述行为
void TestResult::emptyInput_noError()  // 场景_预期
```

---

## 8. 实施优先级

### 第一批（核心单元测试，无外部依赖）

1. 完善 `TestResult`（覆盖所有 Error/Ok 路径）
2. 新增 `TestPipelineContext`（序列化、层操作）
3. 新增 `TestFormatAdapters`（TextGrid/CSV/.ds/.lab 各种正常/异常输入）

### 第二批（领域逻辑）

4. 新增 `TestDsProject` / `TestDsDocument`（工程文件读写）
5. 新增 `TestTaskProcessorRegistry`（注册、校验）

### 第三批（需要 GUI 或模型）

6. 完善 `TestAppShellIntegration`
7. 新增 UI 组件集成测试

---

**文档版本**: 1.0  
**最后更新**: 2026-05-06
