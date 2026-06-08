#include "PhonemeLayerBuilder.h"

#include <nlohmann/json.hpp>

namespace dstools {


dsfw::LayerData buildBoundaries(const QStringList& items, const QStringList& durs, dsfw::TimePos initialPos) {
    nlohmann::json boundaries = nlohmann::json::array();
    dsfw::TimePos cumPos = initialPos;
    const int count = qMin(items.size(), durs.size());
    for (int i = 0; i < count; ++i) {
        boundaries.push_back({{"id", i + 1}, {"pos", cumPos}, {"text", items[i].toStdString()}});
        cumPos += dsfw::secToUs(durs[i].toDouble());
    }
    boundaries.push_back({{"id", count + 1}, {"pos", cumPos}, {"text", ""}});
    return dsfw::LayerData::fromJson(boundaries);
}

}  // namespace dstools
