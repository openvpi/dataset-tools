#pragma once

/// @file TaskTypes.h
/// @brief Core data types for the task processor framework.

#include <nlohmann/json.hpp>

#include <QString>
#include <QStringList>

#include <functional>
#include <map>
#include <string>

namespace dstools {

/// @brief Engine-specific configuration (replaces dynamic_cast + non-interface setters).
/// @see Essentia's Configurable + DiffSinger's filter_kwargs pattern.
using ProcessorConfig = nlohmann::json;

/// @brief Describes a named I/O slot with its layer category.
struct SlotSpec {
    QString name;      ///< Slot name (e.g. "grapheme", "pitch").
    QString category;  ///< Layer category in .dstext.
};

/// @brief Declares a processor's task type and I/O contract.
struct TaskSpec {
    QString taskName;              ///< Task type identifier (e.g. "phoneme_alignment").
    std::vector<SlotSpec> inputs;  ///< Required input slots.
    std::vector<SlotSpec> outputs; ///< Produced output slots.
};

/// @brief Single-item processing input (interactive mode).
struct TaskInput {
    QString audioPath;                         ///< Path to audio file.
    std::map<QString, nlohmann::json> layers;  ///< slot name → layer data (JSON).
    ProcessorConfig config;                    ///< Engine-specific parameters.
};

/// @brief Single-item processing output.
struct TaskOutput {
    std::map<QString, nlohmann::json> layers;  ///< slot name → layer data (JSON).
};

/// @brief Batch processing input (pipeline mode).
struct BatchInput {
    QString workingDir;      ///< Project root directory.
    ProcessorConfig config;  ///< Engine-specific parameters.
};

/// @brief Batch processing output.
struct BatchOutput {
    int processedCount = 0;  ///< Number of items successfully processed.
    int failedCount = 0;     ///< Number of items that failed.
    QString outputPath;      ///< Path to output directory or file.
};

/// @brief Progress callback for batch operations.
/// @param current Current item index.
/// @param total Total number of items.
/// @param item Description of current item.
using ProgressCallback = std::function<void(int current, int total, const QString &item)>;

} // namespace dstools
