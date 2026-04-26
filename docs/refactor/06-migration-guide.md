# 06 — 迁移步骤与验证

**Version**: 3.0  
**Date**: 2026-04-26  
**前置**: 01 ~ 05 全部文档  
**变更**: v3.0 删除一键串联步骤；明确 SlicerPage 不继承 TaskWindow；补充 nativeEvent 适配。

---

## 1. 总体时间线

```
Phase 0: 准备（0.5天）
  │
Phase 1: Core + Audio + Widgets（3天）
  │  里程碑: PlayWidget 可编译运行
  │
Phase 2: Inference 层重组（2天）
  │  里程碑: 所有推理测试通过
  │
Phase 3: DatasetPipeline 构建（3天）
  │  里程碑: 三个 Tab 页均可独立运行
  │
Phase 4: 独立 EXE 迁移（2天）
  │  里程碑: MinLabel / SlurCutter / GameInfer 接入共享样式
  │
Phase 5: Bug 清零 + 现代化（2天）
  │  里程碑: ISSUES.md 43 项全部 resolved
  │
Phase 6: UI 美化（2天）
  │  里程碑: 暗色主题应用，F0Widget 新色板
  │
Phase 7: 测试与验收（1.5天）
  │  里程碑: 所有验证清单通过
  │
总计: ~16 天
```

---

## 2. Phase 0: 准备

1. **创建分支**: `git checkout -b refactor/pipeline-and-style`
2. **确认当前构建通过**
3. **创建新目录结构**:
   ```
   mkdir src/core src/audio src/widgets
   mkdir src/infer/common
   mkdir src/apps/pipeline src/apps/pipeline/slicer src/apps/pipeline/lyricfa src/apps/pipeline/hubertfa
   mkdir resources/themes resources/icons
   ```
4. **移动构建脚本**:
   ```bash
   git mv scripts/cmake/utils.cmake cmake/utils.cmake
   git mv scripts/cmake/winrc.cmake cmake/winrc.cmake
   git mv scripts/cmake/WinResource.rc.in cmake/WinResource.rc.in
   git mv scripts/setup-onnxruntime.cmake cmake/setup-onnxruntime.cmake
   git mv scripts/vcpkg/ports vcpkg/ports
   git mv scripts/vcpkg/triplets vcpkg/triplets
   git mv scripts/vcpkg-manifest/vcpkg.json vcpkg/vcpkg.json
   ```

---

## 3. Phase 1: Core + Audio + Widgets

### Step 1.1: dstools-core (STATIC)

创建 `AppInit`, `Config`, `ErrorHandling`, `ThreadUtils`, `Theme`。

代码来源：从 6 个 `main.cpp` 提取字体/root/crash 初始化。

验证：
- [ ] `dstools-core` 编译通过
- [ ] 最小 main 调用 `AppInit::init()` 正常启动

### Step 1.2: dstools-audio (STATIC)

合并 qsmedia + ffmpegdecoder + sdlplayback → `AudioDecoder` + `AudioPlayback`。

修复 BUG-017/018/019/020/CQ-011。

验证：
- [ ] 编译通过
- [ ] 能打开 WAV/MP3/FLAC 并读取 PCM
- [ ] 能播放音频，CPU 使用率正常（非 100%）

### Step 1.3: dstools-widgets (SHARED/DLL)

合并两份 PlayWidget → 统一 `PlayWidget`（修复 BUG-001/002/010）。
迁移 AsyncTaskWindow → `TaskWindow`。
合并两份 DmlGpuUtils → `GpuSelector`。

验证：
- [ ] `dstools-widgets.dll` 编译通过
- [ ] PlayWidget 基础播放正常
- [ ] PlayWidget 范围播放正常
- [ ] TaskWindow 拖放添加文件正常
- [ ] GpuSelector 能枚举 GPU

---

## 4. Phase 2: Inference 层重组

### Step 2.1: dstools-infer-common

创建 `OnnxEnv` + `ExecutionProvider`。

### Step 2.2: 移动推理库

```bash
git mv src/libs/audio-util src/infer/audio-util
git mv src/libs/game-infer src/infer/game-infer
git mv src/libs/some-infer src/infer/some-infer
git mv src/libs/rmvpe-infer src/infer/rmvpe-infer
git mv src/libs/FunAsr src/infer/funasr
git mv src/libs/onnxruntime src/infer/onnxruntime
```

### Step 2.3: 提取 hubert-infer

从 `src/apps/HubertFA/util/` 提取推理代码到 `src/infer/hubert-infer/`。

### Step 2.4: FunAsr Bug 修复

修复 BUG-015/016/022/023/025。

验证：
- [ ] `TestAudioUtil`, `TestSome`, `TestRmvpe`, `TestGame` 全部通过
- [ ] `hubert-infer` 编译通过

---

## 5. Phase 3: DatasetPipeline 构建

### Step 3.1: 创建 Pipeline 骨架

创建 `PipelineWindow`（仅 QTabWidget + 三个空 Tab 页）。

> **注意**: 不再需要 PipelineRunner。

验证：
- [ ] `DatasetPipeline.exe` 能启动，显示 3 个空 Tab

### Step 3.2: 迁移 AudioSlicer → SlicerPage

1. 将 `src/apps/AudioSlicer/` 代码迁移到 `src/apps/pipeline/slicer/`
2. `MainWindow` → `SlicerPage`（QMainWindow → **QWidget**，**不继承 TaskWindow**）
3. 合并 `mainwindow_ui.h/.cpp` 到 SlicerPage
4. 修复 BUG-006（CSV 索引）
5. `ITaskbarList3` 适配：
   - HWND 通过 `window()->winId()` 获取（而非 `this->winId()`）
   - 原 `nativeEvent` 无法直接用于 QWidget（QWidget 不收到顶层窗口消息），
     需要改用 `QAbstractNativeEventFilter` 注册到 `QApplication`，
     或将 taskbar 初始化移到 `showEvent` 中通过 `window()` 获取 HWND
6. 原 `initStylesMenu()` 删除（由全局 Theme 替代）
7. 原 `slot_oneFinished(filename, listIndex)` 签名保留不变（与 TaskWindow 不同）

验证：
- [ ] Step 1 Tab 可见
- [ ] 拖放添加文件 → 切片 → WAV 输出正常
- [ ] CSV 导出/导入正确
- [ ] 任务栏进度正常

### Step 3.3: 迁移 LyricFA → LyricFAPage

1. 迁移到 `src/apps/pipeline/lyricfa/`
2. `LyricFAWindow` → `LyricFAPage`（继承 TaskWindow）
3. 重命名 `FaTread` → `LyricMatchTask`
4. 修复 BUG-004/005/009

验证：
- [ ] 加载 ASR 模型
- [ ] ASR 识别 → .lab
- [ ] 歌词匹配 → .json
- [ ] 差异高亮

### Step 3.4: 迁移 HubertFA → HubertFAPage

1. 迁移到 `src/apps/pipeline/hubertfa/`
2. `HubertFAWindow` → `HubertFAPage`（继承 TaskWindow）
3. 使用 `GpuSelector`
4. 使用 `hubert-infer` 库
5. 修复 BUG-004/011/021

验证：
- [ ] 加载 HuBERT 模型
- [ ] 音素对齐 → TextGrid
- [ ] GPU 选择

### Step 3.5: 验证分页独立运行

验证每个 Tab 页的独立运行能力。

验证：
- [ ] AudioSlicer Tab: 添加文件 → 切片 → WAV 输出正常
- [ ] LyricFA Tab: 加载模型 → ASR → 歌词匹配正常
- [ ] HubertFA Tab: 加载模型 → 音素对齐 → TextGrid 正常
- [ ] 切换 Tab 不影响其他 Tab 的运行状态
- [ ] 各 Tab 的 Run/Stop/进度条/日志各自独立

---

## 6. Phase 4: 独立 EXE 迁移

### Step 4.1: MinLabel

1. `main.cpp` → `AppInit::init()` + `Theme::apply()`
2. 删除自有 `PlayWidget`，改用 `dstools-widgets::PlayWidget`
3. `saveFile` → `bool`（BUG-001）
4. 修复 CQ-006 拼写、BUG-008 QDir
5. 保持 `QMainWindow`，保持自己的菜单栏

验证：
- [ ] 深色主题正确
- [ ] 全部原有功能正常（见 01-architecture §8 验证清单）

### Step 4.2: SlurCutter

1. `main.cpp` → `AppInit::init()` + `Theme::apply()`
2. 删除自有 `PlayWidget`，改用共享 `PlayWidget`（范围播放模式）
3. 拆分 F0Widget → 4 文件
4. 修复 BUG-003/013/024
5. 保持 `QMainWindow`

验证：
- [ ] 深色主题正确（特别是 F0Widget 自绘区域）
- [ ] 全部原有功能正常

### Step 4.3: GameInfer

1. `main.cpp` → `AppInit::init()` + `Theme::apply()`
2. 删除自有 `DmlGpuUtils`，改用 `GpuSelector`
3. 修复 BUG-012
4. 保持 `QMainWindow`

验证：
- [ ] 深色主题正确
- [ ] GPU 选择正常
- [ ] MIDI 推理正常

### Step 4.4: 删除旧代码

```bash
# 被 Pipeline 替代的独立应用入口
rm src/apps/AudioSlicer/main.cpp    # (代码已迁移到 pipeline/slicer)
rm src/apps/LyricFA/main.cpp         # (代码已迁移到 pipeline/lyricfa)
rm src/apps/HubertFA/main.cpp        # (代码已迁移到 pipeline/hubertfa)

# 被合并的库
rm -rf src/libs/qsmedia
rm -rf src/libs/ffmpegdecoder
rm -rf src/libs/sdlplayback
rm -rf src/libs/AsyncTaskWindow

# 被共享组件替代的重复代码
rm src/apps/MinLabel/gui/PlayWidget.*
rm src/apps/SlurCutter/gui/PlayWidget.*
rm src/apps/GameInfer/utils/DmlGpuUtils.*
rm src/apps/HubertFA/util/DmlGpuUtils.*

# 被 hubert-infer 替代的
rm src/apps/HubertFA/util/Hfa.*
rm src/apps/HubertFA/util/HfaModel.*
rm src/apps/HubertFA/util/AlignmentDecoder.*
rm src/apps/HubertFA/util/NonLexicalDecoder.*
rm src/apps/HubertFA/util/DictionaryG2P.*
rm src/apps/HubertFA/util/AlignWord.*

# 旧 UI 文件
rm src/apps/AudioSlicer/slicer/mainwindow_ui.*

# 临时脚本
rm setup-vcpkg-temp.bat
```

---

## 7. Phase 5: Bug 清零 + 现代化

在 Phase 1-4 中已修复大部分 Bug。此阶段处理剩余项：

1. BUG-016: Vocab str2int 边界检查
2. BUG-023: ftell 截断
3. BUG-025: memcpy 替代类型双关
4. CQ-003: `tr()` 包裹全项目 UI 字符串
4. CQ-004: `foreach` → 范围 for（8 处）
5. CQ-005: 循环内 regex → `static const`（3 处）
6. CQ-007: 清理不雅注释
7. CQ-008: 遗留裸指针检查
8. CQ-010: FunAsr 命名常量
9. FEAT-001: F0Widget UndoStack（QUndoCommand）
10. FEAT-003: AudioSlicer 参数校验
11. FEAT-006: 新增单元测试（TestSlicer, TestSequenceAligner, TestAlignmentDecoder）

---

## 8. Phase 6: UI 美化

1. 编写 `resources/themes/dark.qss`（按 04-ui-theming.md §3）
2. 编写 `resources/themes/light.qss`
3. 实现 `Theme::apply()` 加载 QSS
4. 更新 `PianoRollRenderer` 色板（按 04-ui-theming.md §4）
5. 添加 SVG 图标到 `resources/icons/`
6. 各应用 UI 微调（按 04-ui-theming.md §7）

验证：
- [ ] 4 个 EXE 深色主题下所有控件可辨识且风格一致
- [ ] 亮色主题切换正常
- [ ] F0Widget 新色板正确
- [ ] 图标在不同 DPI 下清晰

---

## 9. Phase 7: 最终验证

### 9.1 构建验证

```bash
rm -rf build
cmake -B build -G Ninja \
    -DCMAKE_PREFIX_PATH=$QT_DIR \
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build --target all
cmake --build build --target install
```

- [ ] 编译零 warning
- [ ] 安装目录: 4 个 EXE + dstools-widgets.dll + 推理 DLL
- [ ] windeployqt 对 4 个 EXE 均成功

### 9.2 自动测试

- [ ] TestAudioUtil, TestSome, TestRmvpe, TestGame 通过
- [ ] TestSlicer, TestSequenceAligner, TestAlignmentDecoder 通过（新）

### 9.3 手动测试矩阵

| 测试场景 | 应用 | 预期 |
|----------|------|------|
| 启动 DatasetPipeline.exe | Pipeline | 3 个 Tab，深色主题 |
| AudioSlicer Tab 切片 | Pipeline | WAV 输出正确 |
| LyricFA Tab ASR | Pipeline | .lab 正确 |
| HubertFA Tab 对齐 | Pipeline | TextGrid 正确 |
| Tab 切换不干扰运行 | Pipeline | 正在运行的 Tab 不受影响 |
| 中途 Stop | Pipeline | 当前 Tab 正确停止 |
| 启动 MinLabel.exe | MinLabel | 深色主题，播放正常 |
| 标注 + 保存 | MinLabel | JSON 正确 |
| G2P 中文/日语/粤语 | MinLabel | 拼音正确 |
| 启动 SlurCutter.exe | SlurCutter | 深色主题，F0 显示正常 |
| 分割/合并/撤销 | SlurCutter | 操作正确 |
| 范围播放 | SlurCutter | 播放头同步 |
| 启动 GameInfer.exe | GameInfer | 深色主题，GPU 列表正常 |
| 推理 → MIDI | GameInfer | MIDI 正确 |
| 拔掉音频设备启动 | MinLabel/SlurCutter | 播放禁用，不崩溃 |
| 4 个 EXE 字体/配色 | 全部 | 视觉一致 |

### 9.4 性能基准

| 指标 | 要求 |
|------|------|
| Pipeline 启动时间 | < 3s |
| MinLabel/SlurCutter/GameInfer 启动时间 | < 2s |
| 内存占用（空闲） | < 80MB (Pipeline), < 50MB (独立工具) |
| F0Widget 渲染 FPS | >= 60fps |
| 推理延迟 | 无退化 |

---

## 10. 回滚方案

每个 Phase 完成后打 tag：

```bash
git tag refactor/phase-0-prep
git tag refactor/phase-1-core-audio-widgets
git tag refactor/phase-2-inference
git tag refactor/phase-3-pipeline
git tag refactor/phase-4-standalone
git tag refactor/phase-5-bugfix
git tag refactor/phase-6-ui
git tag refactor/phase-7-final
```

---

## 11. 文档更新

重构完成后：

- [ ] `README.md` — 更新: 4 个 EXE 说明，Pipeline 用法
- [ ] `docs/PRD.md` — 更新架构图、目录结构
- [ ] `docs/ISSUES.md` — 所有 Issue 标记 Resolved
- [ ] `.github/workflows/windows-rel-build.yml` — 构建 4 个目标
- [ ] 删除 `docs/REFACTORING.md`（旧版）
