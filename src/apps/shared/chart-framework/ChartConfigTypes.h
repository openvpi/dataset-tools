#pragma once

#include <QString>
#include <QVariant>
#include <vector>

namespace dstools {

enum class ParamType {
    Int,
    Double,
};

struct ChartParamDescriptor {
    QString key;
    QString displayName;
    ParamType type = ParamType::Int;
    QVariant defaultValue;
    QVariant minValue;
    QVariant maxValue;
    QString suffix;
    int decimals = 0;
    QString tooltip;
};

struct ChartConfigDescriptor {
    QString chartId;
    QString displayName;
    std::vector<ChartParamDescriptor> params;
};

} // namespace dstools