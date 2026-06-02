#pragma once

#include <dstools/DsPitchDocument.h>
#include <dstools/DsTextTypes.h>
#include <dstools/Result.h>
#include <dstools/TimePos.h>
#include <QList>
#include <memory>

namespace dstools {

class DsTextDocBridge {
public:
    static QList<IntervalLayer> extractIntervalLayers(const DsTextDocument& doc);

    static void mergeIntervalLayers(DsTextDocument& doc, const QList<IntervalLayer>& layers);

    static std::shared_ptr<pitchlabeler::DsPitchDocument> toPitchDoc(const DsTextDocument& doc,
                                                                     TimePos totalDurationUs);

    [[nodiscard]] static Result<void> fromPitchDoc(DsTextDocument& doc, const pitchlabeler::DsPitchDocument& pdoc);

    [[nodiscard]] static Result<void> verifyLayerRoundtrip(const DsTextDocument& original,
                                                           const DsTextDocument& restored);

    [[nodiscard]] static Result<void> verifyPitchDocRoundtrip(const DsTextDocument& original,
                                                              const pitchlabeler::DsPitchDocument& restored);
};

} // namespace dstools