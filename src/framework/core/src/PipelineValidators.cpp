#include <dsfw/PipelineValidators.h>

namespace dstools {

ValidationCallback makeMinFieldValidator(const QString &layerName,
                                          const QString &fieldName,
                                          double minValue) {
    return [layerName, fieldName, minValue](const PipelineContext &ctx, const TaskSpec &, QString &reason) {
        auto it = ctx.layers.find(layerName);
        if (it == ctx.layers.end()) {
            return PipelineContext::Status::Active;
        }
        auto j = it->second.toJson();
        if (!j.contains(fieldName.toStdString())) {
            return PipelineContext::Status::Active;
        }
        double val = j[fieldName.toStdString()].get<double>();
        if (val < minValue) {
            reason = QStringLiteral("%1.%2 = %3 < %4")
                         .arg(layerName, fieldName)
                         .arg(val)
                         .arg(minValue);
            return PipelineContext::Status::Discarded;
        }
        return PipelineContext::Status::Active;
    };
}

ValidationCallback makeMaxFieldValidator(const QString &layerName,
                                          const QString &fieldName,
                                          double maxValue) {
    return [layerName, fieldName, maxValue](const PipelineContext &ctx, const TaskSpec &, QString &reason) {
        auto it = ctx.layers.find(layerName);
        if (it == ctx.layers.end()) {
            return PipelineContext::Status::Active;
        }
        auto j = it->second.toJson();
        if (!j.contains(fieldName.toStdString())) {
            return PipelineContext::Status::Active;
        }
        double val = j[fieldName.toStdString()].get<double>();
        if (val > maxValue) {
            reason = QStringLiteral("%1.%2 = %3 > %4")
                         .arg(layerName, fieldName)
                         .arg(val)
                         .arg(maxValue);
            return PipelineContext::Status::Discarded;
        }
        return PipelineContext::Status::Active;
    };
}

ValidationCallback makeRangeFieldValidator(const QString &layerName,
                                            const QString &fieldName,
                                            double minValue,
                                            double maxValue) {
    return [layerName, fieldName, minValue, maxValue](const PipelineContext &ctx, const TaskSpec &, QString &reason) {
        auto it = ctx.layers.find(layerName);
        if (it == ctx.layers.end()) {
            return PipelineContext::Status::Active;
        }
        auto j = it->second.toJson();
        if (!j.contains(fieldName.toStdString())) {
            return PipelineContext::Status::Active;
        }
        double val = j[fieldName.toStdString()].get<double>();
        if (val < minValue || val > maxValue) {
            reason = QStringLiteral("%1.%2 = %3 not in [%4, %5]")
                         .arg(layerName, fieldName)
                         .arg(val)
                         .arg(minValue)
                         .arg(maxValue);
            return PipelineContext::Status::Discarded;
        }
        return PipelineContext::Status::Active;
    };
}

} // namespace dstools