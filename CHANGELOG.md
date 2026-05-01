# Changelog

All notable changes to the dsfw framework and dstools project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- **T-6.1.2** Merge duplicate Slicer implementations: removed Qt-wrapper `Slicer` class from `libs/slicer/`, `SliceJob` now uses `ISlicerService` via `ServiceLocator` instead of directly instantiating the Qt wrapper. Added `maxSilKept` parameter to `ISlicerService::slice()`. Registered `SlicerService` with `ServiceLocator` in pipeline and CLI apps.
- **T-6.3.4** Thread-safety fix for `registerModelType()` and `registerDocumentFormat()`: added `std::mutex` to protect concurrent access to the internal registry. Created `IDocumentFormat` interface and `DocumentFormatId` registration pattern (matching `ModelTypeId`).

## [0.5.0] - 2026-04-30

### Added

- **T-5.1.1** Unified error handling with `Result<T>`: all inference layer and framework core public APIs now return `Result<T>` instead of `bool + errorMsg`. Added `Ok()`/`Err()` convenience functions.
- **T-5.1.2** Application-layer inference decoupling: UI pages no longer directly `#include` inference library headers. Inference engines managed by domain layer, accessed via `ServiceLocator`.
- **T-5.3.1** CI matrix build: GitHub Actions for Windows (MSVC), macOS (arm64), Linux (GCC). Ccache and vcpkg caching. JUnit XML test reports.
- **T-4.1** Plugin system: `PluginManager` based on `QPluginLoader` with `plugins<T>()` query.
- **T-4.2** Crash handler: Windows MiniDump / Unix signal handler.
- **T-4.3** Auto-update checker via GitHub Releases API.
- **T-4.4** MRU recent files list based on QSettings.
- **T-4.5** New standard widgets: `PropertyEditor`, `SettingsDialog`, `LogViewer`, `ProgressDialog`.

## [0.4.0] - 2026-04-20

### Added

- **T-3.1** Structured logging system: Logger singleton with 6 severity levels, category tags, pluggable sinks.
- **T-3.2** Command / Undo-Redo framework: `ICommand` + `UndoStack` in dsfw-core.
- **T-3.3** Type-safe event bus: `EventBus` based on `std::any` + `std::type_index`.
- **T-3.4** Widget stability audit: null-pointer guards + dsfw-widgets-test unit tests.
- **T-3.5** Common widget migration: dsfw-widgets (SHARED) with 8 common widgets, dstools-widgets retains 4 DS-specific widgets.
- **T-3.6** CLI command-line tool: `dstools-cli` with slice/asr/align/pitch/transcribe subcommands.

## [0.3.0] - 2026-04-10

### Added

- **T-2.1** Reorganized dsfw directory structure and exports.
- **T-2.2** Versioned dsfw public API: `DSFW_VERSION` macro + `DSFW_EXPORT` symbol export.
- **T-2.3** Standardized model config loading: `OnnxModelBase::loadConfig()` + `onConfigLoaded()` virtual methods.
- **T-2.4** `IInferenceEngine` ↔ `IModelProvider` bridge: `InferenceModelProvider<Engine>` template.
- **T-2.5** FunASR adapter: implements `IInferenceEngine` without modifying vendor source.
- **T-2.6** Pipeline backend extraction: `SlicerService`, `AsrService`, `AlignmentService` implementations.

## [0.2.0] - 2026-04-05

### Added

- **T-1.1** `ModelManager` migrated to domain layer. `IModelManager` interface in dsfw-core, implementation in dstools-domain.
- **T-1.2** dsfw-core split: dsfw-base created (Qt-free static library with `JsonHelper`).
- **T-1.3** Enhanced `IInferenceEngine`: added `load()`/`unload()`/`estimatedMemoryBytes()` methods.
- **T-1.4** Created `OnnxModelBase`: `OnnxModelBase` + `CancellableOnnxModel` implemented.
- **T-1.5** Backend service interfaces: `ISlicerService`, `IAlignmentService`, `IAsrService`, `IPitchService`, `ITranscriptionService`.

## [0.1.0] - 2026-04-01

### Added

- **T-0.1** Generalized `ModelType` enum to `ModelTypeId` integer ID + string registry. `DocumentFormat` generalized to `DocumentFormatId`.
- **T-0.2** dsfw-core Qt::Gui removal: dsfw-core only links Qt::Core + Qt::Network.
- **T-0.3** Inference common layer Qt removal: dstools-infer-common no longer depends on Qt::Core.
- **T-0.4** Pipeline relative path fix: slicer/lyricfa/hubertfa as STATIC libraries.
- **T-0.5** DS business leak audit: identified 4 leaks, all scheduled for fix in subsequent phases.
