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

} // namespace dstools
