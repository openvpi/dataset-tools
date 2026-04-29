# Phase 1 门控验证报告

**日期**: 2026-04-29  
**验证环境**: Windows 11, MSVC 2022, cmake-build-release  
**验证方法**: 自动化代码检查 + 构建产物确认（功能回归需手动执行）

---

## 门控结果总览

| 门控 | 条件 | 结果 | 说明 |
|------|------|------|------|
| **G1** | CI 构建成功 | ⚠️ 部分 | Windows Release 构建产物完整（5 EXE + 6 DLL + 4 Test）；macOS/Linux CI 尚未配置 |
| **G2** | 功能回归通过 | ⏳ 待手动 | 需按 05-gate-criteria.md §2 逐项手动验证 |
| **G3** | align 验证报告完整 | ✅ 通过 | spike-game-align-result.md 含结论 + 覆盖度 + 测试情况 + Phase 2 影响 |
| **G4** | schema 决策文档化 | ✅ 通过 | decision-labelvoice-schema.md 含选项 + 理由 + 约束 + 后续计划 |
| **G5** | 无新增警告 | ✅ 通过 | `cmake --build` 输出 0 warnings |
| **G6** | API doxygen 注释 | ✅ 通过 | 7/7 头文件均有 `///` 注释（ViewportController.h 和 Hfa.h 本次补充） |

---

## G1: 构建验证

### 构建产物完整性

| 类型 | 文件 | 状态 |
|------|------|------|
| 应用 | DatasetPipeline.exe | ✅ 2026-04-29 08:33 |
| 应用 | MinLabel.exe | ✅ 2026-04-29 08:33 |
| 应用 | PhonemeLabeler.exe | ✅ 2026-04-29 08:33 |
| 应用 | PitchLabeler.exe | ✅ 2026-04-29 08:33 |
| 应用 | GameInfer.exe | ✅ 2026-04-29 08:33 |
| 共享库 | dstools-widgets.dll | ✅ |
| 共享库 | audio-util.dll | ✅ |
| 共享库 | game-infer.dll | ✅ |
| 共享库 | some-infer.dll | ✅ |
| 共享库 | rmvpe-infer.dll | ✅ |
| 共享库 | hubert-infer.dll | ✅ (新增) |
| 测试 | TestGame.exe | ✅ |
| 测试 | TestRmvpe.exe | ✅ |
| 测试 | TestSome.exe | ✅ |
| 测试 | TestAudioUtil.exe | ✅ |

### 平台覆盖

| 平台 | 状态 | 备注 |
|------|------|------|
| Windows 11 / MSVC 2022 | ✅ 本地构建通过 | cmake-build-release |
| macOS | ❌ CI 未配置 | 免责：CI 基础设施尚未就绪 |
| Linux | ❌ CI 未配置 | 免责：CI 基础设施尚未就绪 |

---

## G2: 功能回归 — 待手动执行

以下清单需人工逐项验证。标记方式：通过 ✅ / 失败 ❌ / 跳过 ⏭️

### DatasetPipeline (5 项)

| # | 验证步骤 | 结果 |
|---|----------|------|
| 1 | 启动，显示 3 标签页 | ☐ |
| 2 | AudioSlicer: 切片 | ☐ |
| 3 | LyricFA: 歌词对齐 | ☐ |
| 4 | HubertFA: 音素对齐 | ☐ |
| 5 | GPU 选择下拉框 | ☐ |

### MinLabel (7 项)

| # | 验证步骤 | 结果 |
|---|----------|------|
| 1 | 打开 .wav + .json 目录 | ☐ |
| 2 | 编辑标注 | ☐ |
| 3 | Ctrl+S 保存 | ☐ |
| 4 | kill 后重开（原子写入） | ☐ |
| 5 | 快捷键编辑对话框 | ☐ |
| 6 | 修改快捷键生效 | ☐ |
| 7 | 重启后快捷键持久化 | ☐ |

### PhonemeLabeler (8 项)

| # | 验证步骤 | 结果 |
|---|----------|------|
| 1 | 打开 .TextGrid 目录 | ☐ |
| 2 | 波形/频谱/能量/层视图 | ☐ |
| 3 | 拖拽音素边界 | ☐ |
| 4 | Undo / Redo | ☐ |
| 5 | 滚轮缩放（同步） | ☐ |
| 6 | 水平拖拽滚动（同步） | ☐ |
| 7 | 快捷键编辑 | ☐ |
| 8 | 快捷键持久化 | ☐ |

### PitchLabeler (10 项)

| # | 验证步骤 | 结果 |
|---|----------|------|
| 1 | 打开 .ds 目录 | ☐ |
| 2 | 钢琴卷帘 + F0 曲线 | ☐ |
| 3 | 音符名称显示 | ☐ |
| 4 | 打开 song_mp3.ds | ☐ |
| 5 | Select 工具 | ☐ |
| 6 | Modulation 工具 | ☐ |
| 7 | Undo / Redo | ☐ |
| 8 | 保存 → 重开 | ☐ |
| 9 | 缩放/滚动 | ☐ |
| 10 | 快捷键编辑 | ☐ |

### GameInfer (4 项)

| # | 验证步骤 | 结果 |
|---|----------|------|
| 1 | 启动 | ☐ |
| 2 | GPU 下拉框 | ☐ |
| 3 | 音频推理 | ☐ |
| 4 | CPU 模式 | ☐ |

---

## G3: 调研文档 — align 验证报告

**文件**: `spike-game-align-result.md` ✅ 存在

| 必须包含 | 状态 |
|---------|------|
| 验证结论 | ✅ 结论 B — 核心完整，需集成补充 |
| 现有实现覆盖度 | ✅ 8 组件覆盖度表 |
| 测试情况 | ✅ 明确指出 0 覆盖 |
| Phase 2 影响 | ✅ TASK-2.8 缩减至 ~1 天 |

---

## G4: 调研文档 — labelvoice schema 决策

**文件**: `decision-labelvoice-schema.md` ✅ 存在

| 必须包含 | 状态 |
|---------|------|
| 选定选项 | ✅ 选项 A |
| 理由 | ✅ §2 (4 点支持 + 排除 B/C) |
| Phase 2 约束 | ✅ §3 (6 条约束 C1-C6) |
| .dstext 后续计划 | ✅ §5 (3 阶段路径) |

---

## G5: 编译警告

```
cmake --build ... --target all 2>&1 | Select-String "warning" | Measure-Object
→ Count: 0
```

✅ 零新增警告。

---

## G6: API doxygen 注释

| 头文件 | `///` 行数 | 阈值 | 结果 |
|--------|-----------|------|------|
| PitchUtils.h | 17 | ≥5 | ✅ |
| AudioFileResolver.h | 12 | ≥3 | ✅ |
| F0Curve.h | 10 | ≥4 | ✅ |
| ShortcutManager.h | 6 | ≥5 | ✅ |
| ViewportController.h | 17 | ≥7 | ✅ (本次补充) |
| BaseFileListPanel.h | 16 | ≥8 | ✅ |
| Hfa.h | 12 | ≥3 | ✅ (本次补充) |

---

## §4: 代码重复消除

| 检查项 | 结果 |
|--------|------|
| PitchLabeler 无本地 freqToMidi 定义 | ✅ PASS |
| PitchLabeler 无本地 parseNoteName 定义 | ✅ PASS |
| MinLabel 无直接 QJsonDocument I/O | ✅ PASS (仅转换辅助函数) |
| GameInfer 无 DmlGpuUtils 文件 | ✅ PASS |
| PhonemeLabeler 无本地 ViewportController | ✅ PASS |
| HubertFA 旧目录已删除 | ✅ PASS |
| PhonemeLabeler 无本地音频查找 | ✅ PASS |

---

## 总结

| 门控 | 判定 |
|------|------|
| G1 | ⚠️ Windows 通过；macOS/Linux CI 未配置（免责） |
| G2 | ⏳ 需手动执行 |
| G3 | ✅ 通过 |
| G4 | ✅ 通过 |
| G5 | ✅ 通过 |
| G6 | ✅ 通过 |

**自动化可验证项全部通过。G2 功能回归需人工执行后更新本报告。**

**建议**: macOS/Linux CI 配置作为 Phase 2 的前置任务，不阻塞当前门控（符合免责条款）。
