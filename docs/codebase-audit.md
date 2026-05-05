# Codebase Audit — Architecture Notes

**Date**: 2026-05-05 (updated)
**Scope**: `dataset-tools` full repository

> **Note**: All original audit action items (Phases 1-5D) have been completed.
> This document retains only the architecture observations for reference.

---

## Architecture Observations

### Layering — Good

```
types (header-only) → framework (base→core→ui-core→widgets)
                    → domain → libs → infer → apps
```

CI verifies independent module builds. `find_package(dsfw)` integration test exists.

### App Layer Structure

`src/apps/shared/` contains shared page components:

| Target | Role |
|--------|------|
| data-sources | Page + Service 统一入口 |
| audio-visualizer | AudioVisualizerContainer + MiniMap + SliceTierLabel |
| phoneme-editor | 音素编辑 UI (含 TierLabelArea, PhonemeTextGridTierLabel) |
| pitch-editor | 音高编辑 UI |
| min-label-editor | 歌词编辑 UI |
| settings-page | 设置 UI |

### Libs Layer Purpose

libs/ 层非薄壳，为 CLI 工具提供无 UI 接口：

| 库 | 用途 |
|----|------|
| hubertfa-lib | alignment decoding + ITaskProcessor 适配 |
| lyricfa-lib | FunASR + 歌词匹配 + ITaskProcessor 适配 |
| slicer-lib | RMS 切片算法 |
| gameinfer-lib | ITaskProcessor 适配（GAME MIDI） |
| rmvpepitch-lib | ITaskProcessor 适配（RMVPE F0） |
| minlabel-lib | MinLabel 业务逻辑 |

### Remaining Architectural Concerns

1. **Thread Safety**: `QtConcurrent::run` captures `this` in Page classes — uses QPointer guard pattern (implemented Phase 1.2)
2. **God Classes**: DsSlicerPage (~945 lines), PitchLabelerPage (~707 lines) — service extraction done for most, DsSlicerPage still large
3. **LabelSuite dual path**: Slice/ASR/Align pages still use old pipeline-pages → data-sources migration (Phase 5A) not yet started

---

## Target Architecture (Current)

```
~33 CMake targets (excluding tests):

Framework (6):  dsfw-base → dsfw-core → dsfw-ui-core → dsfw-widgets + dstools-audio + dstools-infer-common
Domain (1):     dstools-domain
Infer (5):      audio-util, FunAsr, game-infer, hubert-infer, rmvpe-infer
Libs (7):       slicer-lib, lyricfa-lib, hubertfa-lib, gameinfer-lib, rmvpepitch-lib, minlabel-lib, textgrid
App-Shared (6): data-sources, audio-visualizer, phoneme-editor, pitch-editor, min-label-editor, settings-page
Apps (4+):      LabelSuite, DsLabeler, dstools-cli, WidgetGallery
```

---

## Positive Findings

- Clean header hygiene (no `using namespace` in headers)
- No dead code blocks (`#if 0`)
- No stale TODOs in project code
- Good CI (multi-platform, module independence verification, clang-tidy)
- Framework independently consumable via `find_package(dsfw)`
- 32+ test files covering domain + framework core
