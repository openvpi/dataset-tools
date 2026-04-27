# 技术债清单

**Version**: 1.1 | **Date**: 2026-04-27

> 本文档记录项目的**架构级技术债务**和**战略改进项**。
> 具体 Bug 和代码质量问题详见 `bugs.md`。

---

## 架构债

### AD-01: hubert-infer 未提取

HubertFA 的推理代码仍然留在应用层，没有下沉到 `src/infer/` 层。

- **位置**: `src/apps/pipeline/hubertfa/` 下的推理相关代码
- **目标**: 提取到 `src/infer/hubert-infer/`，与 game-infer、some-infer、rmvpe-infer 对齐
- **工作量**: M（1~2天）
- **阻塞**: 无
- **被阻塞**: 无

### AD-02: GameInfer 未迁移到 GpuSelector

GameInfer 仍在使用私有的 DmlGpuUtils，没有切换到 dstools-widgets 中统一的 GpuSelector。

- **位置**: `src/apps/GameInfer/`
- **目标**: 移除私有 DmlGpuUtils，改用 `src/widgets/` 中的 GpuSelector
- **工作量**: S（半天）
- **阻塞**: 无

### AD-03: ErrorHandling 模块缺失

整个项目没有统一的错误处理框架。缺少 Result/Status 类型，各模块自行处理错误，风格不一。

- **目标**: 实现 ErrorHandling 模块，提供 `Result<T>` / `Status` 类型
- **工作量**: M（1~2天设计+实现）
- **阻塞**: CQ-009（详见 bugs.md）依赖此项

### AD-04: ThreadUtils 模块缺失

缺少 `invokeOnMain()` 等跨线程调用工具。各处自行实现 Qt 线程间通信。

- **目标**: 实现 ThreadUtils 模块
- **工作量**: S（半天）
- **阻塞**: 无

### AD-05: Theme System 不完整

当前只有 Dark/Light 两种硬编码模式，没有可扩展的主题系统。

- **位置**: `src/core/` Theme 相关
- **目标**: 实现可扩展的主题模式
- **工作量**: M（1~2天）
- **被阻塞于**: CQ-002（详见 bugs.md，F0Widget 拆分后才能做主题适配）

### AD-06: SVG 图标系统未实现

项目仍使用位图图标，没有 SVG 图标支持。

- **工作量**: S（半天）
- **阻塞**: 无

### AD-07: DPI 适配未完成

高 DPI 显示器上界面缩放有问题。

- **工作量**: M（1~2天，需逐个窗口调整）
- **阻塞**: 无

---

## 代码卫生

### CH-01: HubertFA 旧版目录未清理

`src/apps/HubertFA/` 保留了完整的旧版独立应用（main.cpp + GUI + util/），但已不参与构建（`src/apps/CMakeLists.txt` 不含 `add_subdirectory(HubertFA)`）。

- **位置**: `src/apps/HubertFA/`（约 20 个文件）
- **目标**: AD-01 完成后删除整个目录
- **工作量**: XS（确认 pipeline/hubertfa 不再引用后直接删除）
- **阻塞**: AD-01（hubert-infer 提取）
- **附带问题**: 含备份文件 `util/HfaThread.cpp~`，应立即删除

### CH-02: smoke-test.ps1 检查旧版 EXE

`scripts/smoke-test.ps1` 的 L3 测试仍检查 `AudioSlicer.exe`、`LyricFA.exe`、`HubertFA.exe` 是否存在，但这三个已合并为 `DatasetPipeline.exe`。

- **位置**: `scripts/smoke-test.ps1`
- **目标**: 更新为检查 `DatasetPipeline.exe`
- **工作量**: XS（修改约 5 行）
- **阻塞**: 无

### CH-03: resources/ 主题和图标目录为空

`resources/icons/` 和 `resources/themes/` 目录已创建但未填充任何文件。dark.qss/light.qss 和 SVG 图标文件均不存在。当前 QSS 主题由 `src/core/res/` 内的资源提供。

- **位置**: `resources/`
- **目标**: 填充 QSS 主题文件和 SVG 图标，或删除空目录避免误解
- **工作量**: 填充 QSS: S（半天）；SVG 图标: M（1~2天）
- **阻塞**: AD-06（SVG 图标系统）

### CH-04: scripts/ 含空子目录

`scripts/cmake/` 和 `scripts/vcpkg-manifest/` 为空目录。实际 cmake 工具脚本在 `cmake/`，vcpkg 清单在 `scripts/vcpkg-manifest/`（也为空）。

- **位置**: `scripts/cmake/`, `scripts/vcpkg-manifest/`
- **目标**: 删除空目录或迁移内容
- **工作量**: XS
- **阻塞**: 无

---

## 代码质量 & 库级别问题（交叉引用）

以下项目同时出现在 `bugs.md` 中，此处仅列出 ID 供依赖关系和优先级排序引用。完整描述详见 `bugs.md`。

**代码质量:**
CQ-002, CQ-003, CQ-005, CQ-006a, CQ-006b, CQ-007, CQ-008, CQ-009, CQ-010, CQ-011

**库级别 (Major):**
M-01, M-02, M-03, M-05, M-06, M-11

**库级别 (Minor):**
L-02, L-03, L-04, L-05, L-06, L-07, L-08, L-13

---

## FunAsr 专项

FunAsr 模块集中了全项目约 **36%** 的问题，代码风格为 C 风格，与项目整体的现代 C++ 方向严重不符。

**问题清单汇总：**

1. **CQ-008**: 无 RAII，裸指针满天飞（详见 bugs.md）
2. **CQ-010**: 魔法数字（详见 bugs.md）
3. **L-05**: 死代码 `tmp.h`（详见 bugs.md）
4. **L-06**: 宏定义在命名空间内部（详见 bugs.md）
5. 无异常安全保证
6. 无边界检查
7. 无 NULL 检查

**建议方案：**

- **短期**: 修复 L-05、L-06 等低成本项，添加基本的 NULL 检查
- **长期**: 评估替换为 whisper.cpp 等替代 ASR 库的可行性。如果替换成本可接受，整体替换比逐项修复更划算

**替换评估要点：**
- whisper.cpp 是否满足当前 ASR 需求（中文语音识别精度、ONNX Runtime 兼容性）
- 接口适配工作量
- 模型转换成本

---

## 依赖关系

```
CQ-002 (F0Widget拆分) ← 详见 bugs.md
  ├── 解锁 → AD-05 (Theme适配)
  └── 顺带 → CQ-007 (不当注释)

AD-03 (ErrorHandling模块)
  └── 解锁 → CQ-009 (统一错误处理)

CQ-003 (i18n) ← 详见 bugs.md
  └── 包含 → L-02, M-03
```

---

## 按优先级排序

### 工作量估算说明

| 级别 | 时间 |
|------|------|
| XS | < 30分钟 |
| S | 半天 |
| M | 1~2天 |
| L | 2~3天 |
| XL | 3~5天 |

### P0 (高优先，快速修复)

| 项目 | 工作量 | 说明 |
|------|--------|------|
| CQ-005 | XS | 详见 bugs.md |
| CQ-006b | XS | 详见 bugs.md |
| CQ-007 | XS | 详见 bugs.md |
| CQ-011 | XS | 详见 bugs.md |
| M-01, M-02, M-06, M-11 | XS each | 详见 bugs.md |
| L-04, L-05, L-07 | XS each | 详见 bugs.md |
| CH-01 (HfaThread.cpp~ 备份) | XS | 立即删除备份文件 |
| CH-02 | XS | smoke-test.ps1 更新 |
| CH-04 | XS | 删除空目录 |

### P1 (中优先，有架构价值)

| 项目 | 工作量 | 说明 |
|------|--------|------|
| AD-02 | S | GameInfer 迁移 GpuSelector |
| AD-01 | M | hubert-infer 提取 |
| CQ-006a | S | 详见 bugs.md |
| M-05 | S | 详见 bugs.md |
| L-13 | XS | 详见 bugs.md |
| CH-01 (HubertFA 目录清理) | XS | 依赖 AD-01 |
| CH-03 | S~M | 填充 resources/ |

### P2 (重要，工作量大)

| 项目 | 工作量 | 依赖 |
|------|--------|------|
| CQ-002 | L | 无，但阻塞 AD-05（详见 bugs.md） |
| AD-03 | M | 无，但阻塞 CQ-009 |
| CQ-008 | L | 部分依赖 AD-03（详见 bugs.md） |
| CQ-009 | L | 依赖 AD-03（详见 bugs.md） |

### P3 (长线规划)

| 项目 | 工作量 | 说明 |
|------|--------|------|
| CQ-003 | XL | 详见 bugs.md |
| AD-05 | M | 依赖 CQ-002 |
| AD-06 | S | SVG 图标系统 |
| AD-07 | M | DPI 适配 |
| FunAsr 替换评估 | M | 需要调研+原型验证 |

---

## 总量统计

| 类别 | 项目数 | 预估总工作量 |
|------|--------|-------------|
| 架构债 (本文档独有) | 7 | ~8天 |
| 代码质量 (详见 bugs.md) | 10 | ~12天 |
| 库级别 (详见 bugs.md) | 14 | ~3天 |
| FunAsr 专项 | (含上述) | 另需评估 |
| 代码卫生 (本文档新增) | 4 | ~2天 |
| **合计** | **35项** | **~25天** |

> 以上为单人全职工作量估算，实际可并行处理。P0 项目建议在日常开发中穿插完成。
