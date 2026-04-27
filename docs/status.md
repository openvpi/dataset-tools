# 重构进度追踪

**最后更新**: 2026-04-27

本文档追踪 `docs/` 下各设计文档的实现状态。

---

## 设计偏离说明

### 配置系统（module-spec.md §1.2）

原始设计为 `dstools::Config`，基于 QSettings INI 格式的薄封装。实际实现已**超越原始设计**，演进为 `dstools::AppSettings`：

| 原始设计 (Config) | 实际实现 (AppSettings) |
|---|---|
| QSettings/INI 后端 | nlohmann::json 后端 |
| 字符串 key | `SettingsKey<T>` 强类型 key |
| 无变更通知 | per-key observer + `keyChanged` 信号 |
| 手动 `sync()` | 即时原子写入 (QSaveFile) |
| `settings()` 逃生舱口 | 无逃生舱口，完全类型安全 |

配置文件格式从 `.ini` 变为 `.json`：
```
config/
├── DatasetPipeline.json
├── MinLabel.json
├── SlurCutter.json
└── GameInfer.json
```

每个 app 的所有 key 定义在集中的 schema 头文件中：
- `src/apps/MinLabel/MinLabelKeys.h` (6 keys)
- `src/apps/SlurCutter/SlurCutterKeys.h` (8 keys)
- `src/apps/GameInfer/GameInferKeys.h` (8 keys)
- `src/apps/pipeline/PipelineKeys.h` (1 key)

---

## architecture.md 进度

| 目标 | 状态 | 说明 |
|------|------|------|
| 4 EXE 拓扑 | ✅ DONE | Pipeline + MinLabel + SlurCutter + GameInfer |
| dstools-core STATIC | ✅ DONE | AppInit, AppSettings, Theme |
| dstools-audio STATIC | ✅ DONE | AudioDecoder, AudioPlayback, WaveFormat |
| dstools-widgets SHARED | ✅ DONE | PlayWidget, TaskWindow, GpuSelector |
| dstools-infer-common STATIC | ✅ DONE | OnnxEnv, ExecutionProvider |
| Pipeline 三 Tab 页独立 | ✅ DONE | Slicer/LyricFA/HubertFA |
| 移除 qsmedia 插件系统 | ✅ DONE | |
| 合并两个 PlayWidget | ✅ DONE | 共享 PlayWidget |
| 合并 DmlGpuUtils → GpuSelector | ⚠️ PARTIAL | GpuSelector 已创建；GameInfer/HubertFA 尚未切换 |
| 推理库迁移到 src/infer/ | ✅ DONE | |
| 提取 hubert-infer | ❌ TODO | HubertFA 工具代码仍在 src/apps/HubertFA/util/ |
| cmake/ 脚本迁移 | ✅ DONE | |
| 统一主题 (dark/light QSS) | ✅ DONE | |
| 删除旧 AudioSlicer/LyricFA/HubertFA | ⚠️ PARTIAL | AudioSlicer/LyricFA 已删除；HubertFA 目录仍存在但已不参与构建（不在 apps/CMakeLists.txt 中），待 AD-01 完成后删除 |

---

## module-spec.md 进度

| 模块 | 状态 | 说明 |
|------|------|------|
| AppInit | ✅ DONE | 返回 bool (与文档略有差异) |
| Config → AppSettings | ✅ DONE | 超越原设计，见上方偏离说明 |
| ErrorHandling (Result/Status) | ❌ TODO | 代码中不存在，core/ 仅含 AppInit, AppSettings, Theme, JsonHelper |
| ThreadUtils (invokeOnMain) | ❌ TODO | 代码中不存在 |
| Theme | ✅ DONE | 缺少 System 模式 |
| WaveFormat | ✅ DONE | |
| AudioDecoder | ✅ DONE | |
| AudioPlayback | ✅ DONE | |
| PlayWidget | ✅ DONE | |
| TaskWindow | ✅ DONE | |
| GpuSelector | ✅ DONE | |
| OnnxEnv | ✅ DONE | |
| ExecutionProvider | ✅ DONE | |
| JsonHelper | ✅ DONE | 文档中未列出但已实现（安全 JSON 封装） |
| hubert-infer | ❌ TODO | |

---

## bugs.md 进度

### 已修复
BUG-001, 004, 005, 006, 009, 010, 011, 012, 014, 015, 016, 017, 023,
CQ-001, CQ-004, CQ-006(covertAction)

### 部分修复
BUG-003 (F0Widget未拆分), BUG-007 (unique_ptr但hubert-infer未提取), BUG-008 (需验证)

### 未修复
BUG-013, 018, 019, 020, 021, 022, 024, 025,
CQ-002 (F0Widget拆分), CQ-003 (i18n), CQ-005 (静态regex), CQ-006(FaTread重命名),
CQ-007, CQ-008, CQ-009, CQ-010, CQ-011,
FEAT-001 (UndoStack), FEAT-003, FEAT-006

---

## ui-theming.md 进度

| 目标 | 状态 |
|------|------|
| Dark/Light QSS 主题文件 | ⚠️ PARTIAL | Theme 类已实现；`resources/themes/` 目录已创建但为空，QSS 当前由 core/res/ 内资源提供 |
| Theme::palette() 语义色板 | ✅ DONE |
| SVG 图标系统 | ❌ TODO | `resources/icons/` 目录已创建但为空 |
| F0Widget 主题适配 | ❌ TODO (依赖 F0Widget 拆分) |
| DPI 适配 | ❌ TODO |
| 各 app UI 改进建议 | ❌ TODO |
| 主题选择持久化 | ⚠️ PARTIAL (AppSettings 支持，无 UI 切换) |

---

## architecture.md (unified-app) 进度

| 目标 | 状态 |
|------|------|
| PipelineWindow + QTabWidget | ✅ DONE |
| SlicerPage (不继承 TaskWindow) | ✅ DONE |
| LyricFAPage / HubertFAPage | ✅ DONE |
| 各 Tab 独立 Run/Stop/进度/日志 | ✅ DONE |
| main.cpp 统一模式 (AppInit + Theme) | ✅ DONE (全部 4 个 app) |
| 每 EXE 独立配置文件 | ✅ DONE (JSON 格式) |
| dstools-widgets SHARED + DLL 导出 | ✅ DONE |
| 删除旧独立 EXE | ✅ DONE (AudioSlicer/LyricFA) |

---

## Migration Phase 进度

> 原 06-migration-guide.md 已删除，其剩余有用内容保留于此。

| Phase | 状态 | 说明 |
|-------|------|------|
| Phase 0: 准备 | ✅ DONE | |
| Phase 1: Core + Audio + Widgets | ✅ DONE | |
| Phase 2: Inference 层重组 | ⚠️ PARTIAL | hubert-infer 未提取 |
| Phase 3: DatasetPipeline | ✅ DONE | |
| Phase 4: 独立 EXE 迁移 | ⚠️ PARTIAL | GameInfer 未切到 GpuSelector |
| Phase 5: Bug 清零 | ⚠️ PARTIAL | 约 16/43 已修复 |
| Phase 6: UI 美化 | ⚠️ PARTIAL | QSS 主题完成，SVG/F0Widget/细节未做 |
| Phase 7: 测试与验收 | ❌ TODO | |

---

## apps/*.md 各应用文档进度

### apps/DatasetPipeline.md
- ✅ Pipeline 骨架 + 三 Tab 页迁移
- ✅ BUG-005, 006, 009 修复
- ❌ CQ-006 FaTread → LyricMatchTask 类名重命名

### apps/GameInfer.md
- ✅ BUG-012 修复
- ✅ main.cpp 统一模式
- ❌ 替换 DmlGpuUtils → GpuSelector
- ❌ 删除 utils/DmlGpuUtils.*, GpuInfo.h
- ❌ ExecutionProvider 适配

### apps/MinLabel.md
- ✅ 删除旧 PlayWidget，使用共享版本
- ✅ BUG-001 (saveFile → bool, 移除 exit)
- ✅ CQ-006 (covertAction → convertAction)
- ✅ CQ-004 (foreach → range for)
- ❌ BUG-008 (QDir::count)
- ❌ CQ-006 (covertNum → convertNum in TextWidget)

### apps/SlurCutter.md
- ✅ 删除旧 PlayWidget，使用共享版本
- ✅ CQ-004 (foreach → range for)
- ❌ CQ-002 F0Widget 拆分 (1097行 → 4+1 文件)
- ❌ FEAT-001 UndoStack
- ❌ BUG-003, 013, 024
- ❌ PianoRollRenderer 主题适配

---

## CI/CD 进度

| 平台 | 状态 | 说明 |
|------|------|------|
| Windows | ✅ DONE | `.github/workflows/windows-rel-build.yml`：CMake + vcpkg + Qt 6.9.3 + ONNX Runtime DML，tag 触发，自动构建 + 创建 GitHub Release |
| macOS | ❌ TODO | |
| Linux | ❌ TODO | |

---

## 关键剩余工作（按优先级）

### 高优先级
1. **hubert-infer 提取** — 推理代码仍在 src/apps/HubertFA/util/
2. **GameInfer GpuSelector 迁移** — 仍使用私有 DmlGpuUtils
3. **F0Widget 拆分 (CQ-002)** — 1097 行单文件，阻塞主题适配和 bug 修复
4. **清理旧 HubertFA 目录** — `src/apps/HubertFA/` 已脱离构建但仍占仓库空间，含旧版备份文件（`HfaThread.cpp~`）

### 中优先级
5. ErrorHandling / ThreadUtils 模块
6. 剩余 bug 修复 (BUG-013, 018-025)
7. FaTread → LyricMatchTask 类名重命名
8. SVG 图标系统
9. **smoke-test.ps1 更新** — `scripts/smoke-test.ps1` 仍检查 AudioSlicer.exe/LyricFA.exe/HubertFA.exe（已不存在），应改为 DatasetPipeline.exe
10. **resources/ 主题文件填充** — dark.qss / light.qss / SVG 图标文件尚未创建

### 低优先级
11. i18n tr() 包装 (CQ-003)
12. 各 app UI 改进建议 (ui-theming.md §7)
13. DPI 适配
14. FEAT-001 UndoStack
15. Phase 7 测试与验收
