# 技术债清单

**Version**: 1.4 | **Date**: 2026-04-29

> 本文档记录项目的**架构级技术债务**和**战略改进项**。
> 具体 Bug 和代码质量问题详见 `bugs.md`。

---

## 架构债

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

### AD-06: SVG 图标系统未实现

项目仍使用位图图标，没有 SVG 图标支持。

- **工作量**: S（半天）
- **阻塞**: 无

### AD-07: DPI 适配未完成

高 DPI 显示器上界面缩放有问题。

- **工作量**: M（1~2天，需逐个窗口调整）
- **阻塞**: 无

### AD-10: MinLabel JSON I/O 未使用 JsonHelper

MinLabel 自行实现了 `readJsonFile`/`writeJsonFile`（`Common.cpp:223-262`），使用 `QFile` 直接写入，没有原子写入保护。

- **位置**: `src/apps/MinLabel/gui/Common.cpp:223-262`
- **风险**: 保存过程中崩溃或断电会导致文件损坏。`JsonHelper::saveFile` 已通过 temp+rename 解决此问题
- **目标**: MinLabel 的 JSON I/O 改为调用 `JsonHelper`（或直接使用 `DsDocument`），获得原子写入和统一错误处理
- **工作量**: XS（改动量小，收益明确）
- **阻塞**: 无

### AD-11: 快捷键管理缺少集中绑定机制

三个应用都手动维护 action↔快捷键绑定——定义 action、打开编辑器、手动逐个回写快捷键。每个应用 ~30 行重复的绑定/回写代码。

- **位置**:
  - `MinLabel/MainWindow.cpp:56-75` — 快捷键注册
  - `PitchLabeler/MainWindow.cpp:144-182` — 快捷键注册（17 个快捷键条目，已集成 ShortcutEditorWidget）
  - `PhonemeLabeler/` — 使用 `AppSettings.shortcut()` 但尚未集成 ShortcutEditorWidget
  - 各应用的 `applyConfig()` 和 `closeEvent()` 中手动回写
- **风险**: 新增 action 时容易忘记在 `applyConfig`/`closeEvent` 中同步。已经实际发生过一次（commit `3d67219` — `fix(widgets): shortcut edits not taking effect on dialog close`）
- **目标**: 在 `dstools-widgets` 新增 `ShortcutManager`，持有 action↔key 映射表，提供 `applyAll()`/`saveAll()` 一次性生效，消除手动逐个回写的遗漏风险
- **工作量**: S（半天）
- **阻塞**: 无

### AD-12: MinLabel 文件浏览树可提取

MinLabel 的 `QTreeView + QFileSystemModel + 列隐藏 + FileStatusDelegate + 目录打开` 逻辑可提取为公共组件。

- **位置**:
  - `MinLabel/MainWindow.cpp:113-146` — QTreeView + QFileSystemModel 设置，隐藏列、设置过滤器（`*.wav/*.mp3/*.m4a/*.flac`）、创建 `FileStatusDelegate`
- **历史**: 原 SlurCutter 有几乎相同的实现（~60 行逐行重复），SlurCutter 已删除
- **目标**: 在 `dstools-widgets` 抽取为 `AudioFileBrowser` 组件，配置项为文件过滤器和状态检查回调
- **注意**: PitchLabeler 的 `FileListPanel` 是不同设计（QListWidget，浏览 .ds 文件），PhonemeLabeler 也有 `FileListPanel`（QListWidget），两者均不应与 QTreeView 方案强制统一
- **工作量**: S（半天）
- **阻塞**: 无

### 不建议抽取的部分

以下经过评估后认为不值得抽取，记录于此避免重复讨论：

| 项 | 原因 |
|---|---|
| DS 字段解析（note_seq 拆分等） | PitchLabeler 需完整解析为 typed struct，与其他应用需求层次不同，强行统一会过度设计 |
| 主窗口基类 `BaseLabelerWindow` | 三个应用的菜单结构、状态栏、布局差异较大，抽基类会过度耦合 |
| 拖拽打开目录 | 仅 ~20 行实际逻辑（MinLabel 的 dragEnterEvent/dropEvent），提取为 mixin 的成本约等于收益 |

---

## 代码卫生

### CH-03: resources/ 主题目录为空、图标目录部分填充

`resources/themes/` 目录为空（dark.qss/light.qss 不存在，当前 QSS 主题由 `src/core/res/` 内的资源提供）。`resources/icons/` 已有 3 个基础 SVG 文件（pause.svg、play.svg、stop.svg），但远未完整。

- **位置**: `resources/`
- **目标**: 填充 QSS 主题文件到 resources/themes/，补全 SVG 图标
- **工作量**: 填充 QSS: S（半天）；SVG 图标补全: M（1~2天）
- **阻塞**: AD-06（SVG 图标系统）

---

## 代码质量 & 库级别问题（交叉引用）

以下项目同时出现在 `bugs.md` 中，此处仅列出 ID 供依赖关系和优先级排序引用。完整描述详见 `bugs.md`。

**代码质量:**
CQ-003, CQ-006a, CQ-008, CQ-009, CQ-010

**库级别 (Major):**
全部已修复

**库级别 (Minor):**
L-02, L-03, L-08

---

## FunAsr 专项

FunAsr 模块集中了全项目约 **36%** 的问题，代码风格为 C 风格，与项目整体的现代 C++ 方向严重不符。

**问题清单汇总：**

1. **CQ-008**: 无 RAII，裸指针满天飞（详见 bugs.md）
2. **CQ-010**: 魔法数字（详见 bugs.md）
3. **L-05**: ✅ 已修复
4. **L-06**: ✅ 已修复
5. 无异常安全保证
6. 无边界检查（C-03 SpeechWrap 边界检查已修复）
7. 无 NULL 检查（C-04 malloc NULL 检查已修复）

**建议方案：**

- **短期**: ~~修复 L-05、L-06 等低成本项，添加基本的 NULL 检查~~ L-05、L-06、NULL 检查已完成。剩余：CQ-008（RAII）、CQ-010（魔法数字）、异常安全、边界检查
- **长期**: 评估替换为 whisper.cpp 等替代 ASR 库的可行性。如果替换成本可接受，整体替换比逐项修复更划算

**替换评估要点：**
- whisper.cpp 是否满足当前 ASR 需求（中文语音识别精度、ONNX Runtime 兼容性）
- 接口适配工作量
- 模型转换成本

---

## 依赖关系

```
AD-03 (ErrorHandling模块)
  └── 解锁 → CQ-009 (统一错误处理)

CQ-003 (i18n) ← 详见 bugs.md
  └── 包含 → L-02 (M-03 已修复)
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
| AD-10 | XS | MinLabel JSON I/O 迁移到 JsonHelper |

### P1 (中优先，有架构价值)

| 项目 | 工作量 | 说明 |
|------|--------|------|
| AD-11 | S | 快捷键集中管理（已出现过遗漏 Bug） |
| AD-12 | S | 文件浏览树提取（~60行逐行重复） |
| CQ-006a | S | 详见 bugs.md |
| CH-03 | S~M | 填充 resources/ |

### P2 (重要，工作量大)

| 项目 | 工作量 | 依赖 |
|------|--------|------|
| AD-03 | M | 无，但阻塞 CQ-009 |
| CQ-008 | L | 部分依赖 AD-03（详见 bugs.md） |
| CQ-009 | L | 依赖 AD-03（详见 bugs.md） |

### P3 (长线规划)

| 项目 | 工作量 | 说明 |
|------|--------|------|
| CQ-003 | XL | 详见 bugs.md |
| AD-05 | M | 主题系统 |
| AD-06 | S | SVG 图标系统 |
| AD-07 | M | DPI 适配 |
| FunAsr 替换评估 | M | 需要调研+原型验证 |

---

## 总量统计

| 类别 | 项目数 | 预估总工作量 |
|------|--------|-------------|
| 架构债 (本文档独有) | 8 | ~5天 |
| 代码质量 (详见 bugs.md) | 5 | ~8天 |
| 库级别 (详见 bugs.md) | 3 | ~0.5天 |
| FunAsr 专项 | (含上述) | 另需评估 |
| 代码卫生 (本文档新增) | 1 | ~0.5-1.5天 |
| **合计** | **17项** | **~15天** |

> 以上为单人全职工作量估算，实际可并行处理。P0 项目建议在日常开发中穿插完成。
