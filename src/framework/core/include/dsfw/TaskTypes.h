#pragma once

#include "nlohmann/json.hpp"

#include <QString>
#include <QStringList>
#include <dsfw/ConfigTypes.h>
#include <functional>
#include <map>
#include <string>

namespace dsfw {

using ProcessorConfig = ConfigMap;

/// @brief Describes a named I/O slot with its layer category.
struct SlotSpec {
    QString name;     ///< Slot name (e.g. "grapheme", "pitch").
    QString category; ///< Layer category in .dstext.
};

/// @brief Declares a processor's task type and I/O contract.
struct TaskSpec {
    QString taskName;              ///< Task type identifier (e.g. "phoneme_alignment").
    std::vector<SlotSpec> inputs;  ///< Required input slots.
    std::vector<SlotSpec> outputs; ///< Produced output slots.
};

/// @brief Opaque layer data carrier — isolates nlohmann::json from public headers.
///
/// Stores layer content as a JSON string. Consumers parse via toJson() when needed;
/// producers create via fromJson(). This keeps nlohmann::json out of TaskTypes.h and
/// PipelineContext.h, reducing incremental build time for ~30 translation units.
struct LayerData {
    std::string json;

    LayerData() = default;
    explicit LayerData(std::string rawJson) : json(std::move(rawJson)) {}

    static LayerData fromJson(const nlohmann::json& j);
    nlohmann::json toJson() const;

    bool empty() const noexcept { return json.empty(); }

    bool operator==(const LayerData& other) const { return json == other.json; }
    bool operator!=(const LayerData& other) const { return json != other.json; }
};

/// @brief Single-item processing input (interactive mode).
struct TaskInput {
    QString audioPath;                   ///< Path to audio file.
    std::map<QString, LayerData> layers; ///< slot name → layer data.
    ProcessorConfig config;              ///< Engine-specific parameters.
};

/// @brief Single-item processing output.
struct TaskOutput {
    std::map<QString, LayerData> layers; ///< slot name → layer data.
};

} // namespace dsfw

// Backward compatibility
namespace dstools {
    using dsfw::ProcessorConfig;
    using dsfw::SlotSpec;
    using dsfw::TaskSpec;
    using dsfw::LayerData;
    using dsfw::TaskInput;
    using dsfw::TaskOutput;
}
