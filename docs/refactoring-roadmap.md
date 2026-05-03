# 架构重构路线图

> 更新时间：2026-05-03
>
> **原则**: 功能齐全、简单可靠、不过度设计。
>
> **C++ 标准**: C++20 / Qt 6.8+

---

## 当前状态

所有基础架构阶段（Phase 0–N, M.1–M.5）**已完成**。两个应用均可运行：

- **DsLabeler**：7 页面 AppShell，推理集成完整（HubertFA/RMVPE/GAME/FunASR），自动补全，导出完善
- **LabelSuite**：9 页面 AppShell，使用旧 pipeline 页面（独立的 SlicerPage/LyricFAPage/HubertFAPage 等），无推理集成，无自动补全

**核心问题**：两个应用的页面代码完全独立——DsLabeler 在 `src/apps/ds-labeler/` 有自己的 7 个页面实现，LabelSuite 引用 `src/apps/pipeline/`、`src/apps/min-label/`、`src/apps/phoneme-labeler/`、`src/apps/pitch-labeler/` 的旧页面。功能重复、维护双份代码。

---

## Phase P — LabelSuite ↔ DsLabeler 高度统一

### 设计原则

1. **界面一致**：两个应用共享全部页面组件代码，UI 完全相同
2. **LabelSuite = 散装模式**：每一步兼容旧版格式（TextGrid / .lab / .ds / CSV）的文件输入输出
3. **DsLabeler = 流程模式**：合并步骤（FA/MIDI 自动执行）、.dsproj 工程文件驱动、标注流程更连贯
4. **LabelSuite 底层也用 dstext/PipelineContext**：旧版文件通过 FormatAdapter 导入导出
5. **LabelSuite 保留全部独立页面**：HubertFA / GAME / BuildCsv / BuildDs 等在 DsLabeler 里被自动补全的功能，在 LabelSuite 中必须拥有独立界面
6. **LabelSuite 也享受自动补全**：进入 Phone 页自动 FA、进入 Pitch 页自动 F0/MIDI（与 DsLabeler 一致）
7. **LabelSuite 引入 Settings 页**：统一模型配置

### 两个应用的区别

| | LabelSuite | DsLabeler |
|---|---|---|
| 数据模型 | dstext/PipelineContext（统一） | dstext/PipelineContext（统一） |
| 文件格式 | 导入/导出 TextGrid、.lab、.ds、CSV | 仅 dstext（.dsproj 内） |
| 工程文件 | ❌ 无（Set Working Directory） | ✅ .dsproj |
| 自动补全 | ✅ 有（进入 Phone/Pitch 页自动执行） | ✅ 有 |
| 页面 | 10 页（含独立 FA/MIDI/CSV/DS 页 + Settings） | 7 页 |
| 独立 HubertFA 页 | ✅ 保留（手动控制 FA） | ❌ 合并到自动补全 |
| 独立 GAME/MIDI 页 | ✅ 保留（手动控制 MIDI 转录） | ❌ 合并到自动补全 |
| 独立 BuildCsv/BuildDs 页 | ✅ 保留 | ❌ 合并到导出页 |
| Welcome / Export 页 | ❌ | ✅ |

### LabelSuite 页面布局（统一后）

```
LabelSuite (AppShell, 10 页面)
├── Slice        ← 共享切片页
├── ASR          ← 共享歌词识别
├── Label        ← 共享 MinLabelEditor
├── Align        ← 独立 HubertFA 界面（可手动运行 FA）
├── Phone        ← 共享 PhonemeEditor + 自动 FA 补全
├── CSV          ← 独立 BuildCsv 界面
├── MIDI         ← 独立 GAME 界面（可手动运行 MIDI 转录）
├── DS           ← 独立 BuildDs 界面
├── Pitch        ← 共享 PitchEditor + 自动 F0/MIDI 补全
├── Settings     ← 共享 SettingsWidget（持久化到 QSettings）
```

---

### P.B — 统一实施任务清单

#### P.B.1 ISettingsBackend + 共享 SettingsWidget ✅

- [x] 提取 `ISettingsBackend` 接口（`load()` / `save()` 返回 `QJsonObject`）
- [x] `AppSettingsBackend`：基于 QSettings（LabelSuite 用）
- [x] `ProjectSettingsBackend`：基于 .dsproj defaults（DsLabeler 用）
- [x] 提取 DsLabeler 的 `SettingsPage` 到 `src/apps/shared/settings/`
- [x] LabelSuite 注册 Settings 页

#### P.B.2 FileDataSource 改造为 dstext 内核

- [ ] `FileDataSource` 内部使用 `DsTextDocument`
- [ ] 导入：根据文件扩展名选择 FormatAdapter → 转为 dstext
- [ ] 导出：dstext → FormatAdapter → 写回原格式
- [ ] 支持格式：TextGrid、.lab、.ds、CSV、Audacity 标记
- [ ] 移动到 `src/apps/shared/data-sources/`

#### P.B.3 核心页面统一

将 DsLabeler 的页面实现改造为数据源无关，两个应用共享同一页面类：

| 共享页面 | 当前 DsLabeler 实现 | 改造 |
|----------|-------------------|------|
| SlicerPage | DsSlicerPage | 提取为共享，注入 IEditorDataSource |
| MinLabelPage | DsMinLabelPage | 提取为共享 |
| PhonemeLabelerPage | DsPhonemeLabelerPage | 提取为共享（含自动 FA） |
| PitchLabelerPage | DsPitchLabelerPage | 提取为共享（含自动 F0/MIDI） |

- [ ] 每个页面接收 `IEditorDataSource*` + `ISettingsBackend*`
- [ ] SlicerPage 共享
- [ ] MinLabelPage 共享
- [ ] PhonemeLabelerPage 共享
- [ ] PitchLabelerPage 共享

#### P.B.4 LabelSuite 独有页面保留并升级

底层统一到 dstext，保留独立 UI：

- [ ] ASR (LyricFA) — 底层改用 dstext，调用同一推理接口
- [ ] Align (HubertFA) — 底层改用 dstext，复用推理逻辑
- [ ] CSV (BuildCsv) — 底层改用 dstext → CsvAdapter
- [ ] MIDI (GameAlign) — 底层改用 dstext，复用推理逻辑
- [ ] DS (BuildDs) — 底层改用 dstext → DsFileAdapter

#### P.B.5 LabelSuite 自动补全

- [ ] Phone 页进入时自动 FA（有 grapheme 层且无 phoneme 层或 dirty 时）
- [ ] Pitch 页进入时自动 add_ph_num + RMVPE + GAME（缺失或 dirty 时）
- [ ] Toast 通知行为与 DsLabeler 统一

#### P.B.6 旧代码清理

- [ ] 删除 `src/apps/pipeline/slicer/`（→ 共享版）
- [ ] 删除 `src/apps/pipeline/hubertfa/`（→ dstext 版）
- [ ] 删除 `src/apps/pipeline/lyricfa/`（→ dstext 版）
- [ ] 删除 `src/apps/min-label/`（→ 共享版）
- [ ] 删除 `src/apps/phoneme-labeler/`（→ 共享版）
- [ ] 删除 `src/apps/pitch-labeler/`（→ 共享版）
- [ ] 删除 `src/apps/label-suite/TaskWindowAdapter`
- [ ] 更新 CMakeLists.txt

#### P.B.7 集成测试

- [ ] LabelSuite 打开/编辑/保存 TextGrid
- [ ] LabelSuite 打开/编辑/保存 .lab
- [ ] LabelSuite 打开/编辑/保存 .ds
- [ ] LabelSuite 自动补全验证
- [ ] Settings 持久化验证
- [ ] DsLabeler 回归测试

---

## 实施优先级

| 顺序 | 任务 | 复杂度 | 说明 |
|------|------|--------|------|
| 1 | P.B.1 ISettingsBackend + SettingsWidget 共享 | 低 | 解耦配置持久化，LabelSuite 获得 Settings 页 |
| 2 | P.B.2 FileDataSource 改造 | 中 | LabelSuite 底层切换到 dstext |
| 3 | P.B.3 核心页面统一 | 高 | 最大工作量——4 个页面提取为共享 |
| 4 | P.B.5 自动补全 | 低 | 共享页面已含逻辑，LabelSuite 直接获得 |
| 5 | P.B.4 独立页面升级 | 中 | 5 个 LabelSuite 独有页面底层改为 dstext |
| 6 | P.B.6 旧代码清理 | 低 | 删除旧实现 |
| 7 | P.B.7 集成测试 | 中 | 全面验证 |

---

## 关键架构决策 (ADR)

| ADR | 决策 | 理由 |
|-----|------|------|
| 8 | 单仓库模式 | 降低维护复杂度 |
| 43 | int64 微秒时间精度 | 消除浮点累积误差 |
| 46 | LabelSuite + DsLabeler 两个 exe | 通用标注不绑定 DiffSinger |
| 52 | IEditorDataSource 抽象数据源 | 页面代码不感知数据来源 |
| 57 | 层依赖 DAG + per-slice dirty 标记 | 保证数据一致性 |
| 62 | 右键直接播放，不弹菜单 | 标注中最频繁操作应零延迟 |
| 65 | IBoundaryModel 解耦可视化组件 | 切片/音素共享 WaveformWidget/SpectrogramWidget |
| 66 | LabelSuite 底层统一 dstext | 消除数据模型分裂；旧格式通过 FormatAdapter 兼容 |
| 69 | ISettingsBackend 抽象持久化 | Settings UI 共享，后端可切换 QSettings / .dsproj |
| 70 | FileDataSource 内部 dstext + FormatAdapter | 旧格式自动转入/转出，用户无感知 |
| 71 | LabelSuite 保留全部独立页面 + 自动补全 | 手动控制仍可用；自动补全提升效率；两个应用体验一致 |

---

## 关联文档

- [unified-app-design.md](unified-app-design.md) — LabelSuite + DsLabeler 统一应用设计方案
- [ds-format.md](ds-format.md) — .dsproj / .dstext 格式规范
- [task-processor-design.md](task-processor-design.md) — 流水线架构设计
- [framework-architecture.md](framework-architecture.md) — 框架架构
- [phoneme-editor-improvements.md](phoneme-editor-improvements.md) — Phoneme Editor UI 改进记录
