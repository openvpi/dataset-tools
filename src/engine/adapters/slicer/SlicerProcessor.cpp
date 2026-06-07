#include "SlicerProcessor.h"

#include "SlicerService.h"

#include <dsfw/ConfigTypes.h>
#include <dsfw/TaskProcessorRegistry.h>
#include <dsfw/TimePos.h>

namespace dstools {

    using namespace dsfw;

    // Self-register with the task processor registry.
    static TaskProcessorRegistry::Registrar<SlicerProcessor> s_reg(QStringLiteral("audio_slice"),
                                                                   QStringLiteral("slicer"));

    TaskSpec SlicerProcessor::taskSpec() const {
        return {"audio_slice", {}, {{"slices", "slices"}}};
    }

    Result<void> SlicerProcessor::initialize(ModelManager & /*mm*/, const ProcessorConfig & /*config*/) {
        return Ok();
    }

    void SlicerProcessor::release() {
    }

    Result<TaskOutput> SlicerProcessor::process(const TaskInput &input) {
        SlicerService slicer;

        const double threshold = static_cast<double>(configValueDouble(input.config, QStringLiteral("threshold"), -40.0));
        const int minLength = static_cast<int>(configValueInt(input.config, QStringLiteral("minLength"), 5000));
        const int minInterval = static_cast<int>(configValueInt(input.config, QStringLiteral("minInterval"), 300));
        const int hopSize = static_cast<int>(configValueInt(input.config, QStringLiteral("hopSize"), 10));
        const int maxSilKept = static_cast<int>(configValueInt(input.config, QStringLiteral("maxSilKept"), 500));

        auto result = slicer.slice(input.audioPath, threshold, minLength, minInterval, hopSize, maxSilKept);
        if (!result.ok())
            return Err<TaskOutput>(result.error());

        const auto &sr = result.value();

        nlohmann::json slicesJson = nlohmann::json::array();
        int id = 1;
        for (const auto &[startSample, endSample] : sr.chunks) {
            const TimePos inPos = secToUs(static_cast<double>(startSample) / sr.sampleRate);
            const TimePos outPos = secToUs(static_cast<double>(endSample) / sr.sampleRate);
            slicesJson.push_back({
                {"id", QString("%1").arg(id, 3, 10, QChar('0')).toStdString()},
                {"in", inPos},
                {"out", outPos},
                {"status", "active"}
            });
            ++id;
        }

        TaskOutput output;
        output.layers["slices"] = LayerData::fromJson(slicesJson);
        return Ok(std::move(output));
    }

} // namespace dstools
