#pragma once

#include <dsfw/PipelineContext.h>
#include <dsfw/PipelineRunner.h>
#include <dsfw/TaskTypes.h>
#include <QString>

namespace dsfw {

ValidationCallback makeMinFieldValidator(const QString &layerName,
                                          const QString &fieldName,
                                          double minValue);

ValidationCallback makeMaxFieldValidator(const QString &layerName,
                                          const QString &fieldName,
                                          double maxValue);

ValidationCallback makeRangeFieldValidator(const QString &layerName,
                                            const QString &fieldName,
                                            double minValue,
                                            double maxValue);

inline ValidationCallback makeSliceLengthValidator(double minSec, double maxSec) {
    return makeRangeFieldValidator(QStringLiteral("wave"), QStringLiteral("duration"), minSec, maxSec);
}

inline ValidationCallback makeAlignmentQualityValidator(const QString &layerName, double minConfidence) {
    return makeMinFieldValidator(layerName, QStringLiteral("confidence"), minConfidence);
}

inline ValidationCallback makePitchCoverageValidator(const QString &layerName, double minCoverage) {
    return makeMinFieldValidator(layerName, QStringLiteral("coverage"), minCoverage);
}

} // namespace dsfw