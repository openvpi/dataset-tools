# Namespace Migration: dstools → dsfw

## Overview

Root namespace for framework classes: `dsfw`  
Application/domain namespace remains: `dstools`

## Migration Mapping

| Old Location | Old Namespace | New Location | New Namespace |
|---|---|---|---|
| `dstools/AppSettings.h` | `dstools` | `dsfw/AppSettings.h` | `dsfw` |
| `dstools/JsonHelper.h` | `dstools` | `dsfw/JsonHelper.h` | `dsfw` |
| `dstools/AsyncTask.h` | `dstools` | `dsfw/AsyncTask.h` | `dsfw` |
| `dstools/ServiceLocator.h` | `dstools` | `dsfw/ServiceLocator.h` | `dsfw` |
| `dstools/IDocument.h` | `dstools` | `dsfw/IDocument.h` | `dsfw` |
| `dstools/IFileIOProvider.h` | `dstools` | `dsfw/IFileIOProvider.h` | `dsfw` |
| `dstools/LocalFileIOProvider.h` | `dstools` | `dsfw/LocalFileIOProvider.h` | `dsfw` |
| `dstools/IExportFormat.h` | `dstools` | `dsfw/IExportFormat.h` | `dsfw` |
| `dstools/IModelProvider.h` | `dstools` | `dsfw/IModelProvider.h` | `dsfw` |
| `dstools/IModelDownloader.h` | `dstools` | `dsfw/IModelDownloader.h` | `dsfw` |
| `dstools/IQualityMetrics.h` | `dstools` | `dsfw/IQualityMetrics.h` | `dsfw` |
| `dstools/IG2PProvider.h` | `dstools` | `dsfw/IG2PProvider.h` | `dsfw` |

## Domain Classes (remain in dstools namespace)

| Header | Namespace | Location |
|---|---|---|
| `DsDocument.h` | `dstools` | `src/domain/include/dstools/` |
| `DsProject.h` | `dstools` | `src/domain/include/dstools/` |
| `DsItemManager.h` | `dstools` | `src/domain/include/dstools/` |
| `CsvToDsConverter.h` | `dstools` | `src/domain/include/dstools/` |
| `TextGridToCsv.h` | `dstools` | `src/domain/include/dstools/` |
| `TranscriptionCsv.h` | `dstools` | `src/domain/include/dstools/` |
| `TranscriptionPipeline.h` | `dstools` | `src/domain/include/dstools/` |
| `PitchUtils.h` | `dstools` | `src/domain/include/dstools/` |
| `F0Curve.h` | `dstools` | `src/domain/include/dstools/` |
| `PhNumCalculator.h` | `dstools` | `src/domain/include/dstools/` |
| `AudioFileResolver.h` | `dstools` | `src/domain/include/dstools/` |
| `PinyinG2PProvider.h` | `dstools` | `src/domain/include/dstools/` |
| `ExportFormats.h` | `dstools` | `src/domain/include/dstools/` |
| `DsDocumentAdapter.h` | `dstools` | `src/domain/include/dstools/` |

## CMake Targets

| Old Target | New Target | Alias |
|---|---|---|
| `dstools-core` (mixed) | `dsfw-core` (framework) | `dsfw::core` |
| — | `dsfw-ui-core` (UI framework) | `dsfw::ui-core` |
| — | `dstools-domain` (domain) | — |

## Notes

- Framework classes (`dsfw`) have ZERO dependency on domain classes (`dstools`)
- Domain classes depend on framework via `dsfw-core`
- Transition period: forwarding headers at old paths `#include <dsfw/XXX.h>` redirect
