#include "PhonemeLayerBuilder.h"

#include <nlohmann/json.hpp>

namespace dstools {

    LayerData buildBoundaries(const QStringList &items, const QStringList &durs, TimePos initialPos) {
        nlohmann::json boundaries = nlohmann::json::array();
        TimePos cumPos = initialPos;
        const int count = qMin(items.size(), durs.size());
        for (int i = 0; i < count; ++i) {
            boundaries.push_back({
                {"id",   i + 1                 },
                {"pos",  cumPos                },
                {"text", items[i].toStdString()}
            });
            cumPos += secToUs(durs[i].toDouble());
        }
        boundaries.push_back({
            {"id",   count + 1},
            {"pos",  cumPos   },
            {"text", ""       }
        });
        return LayerData::fromJson(boundaries);
    }

} // namespace dstools