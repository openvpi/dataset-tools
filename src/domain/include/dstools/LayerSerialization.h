#pragma once

#include <dsfw/TaskTypes.h>
#include <dstools/LayerTypes.h>
#include <dstools/Result.h>

#include <QString>

namespace dstools {

    Result<LayerDataVariant> parseLayerData(const LayerData &data, const QString &layerType);

    LayerData serializeLayerData(const LayerDataVariant &variant);

} // namespace dstools