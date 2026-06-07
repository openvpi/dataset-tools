#pragma once

#include <dsfw/TaskTypes.h>
#include <dstools/LayerTypes.h>
#include <dsfw/Result.h>

#include <QString>

namespace dstools {

    using namespace dsfw;

    Result<LayerDataVariant> parseLayerData(const LayerData &data, const QString &layerType);

    LayerData serializeLayerData(const LayerDataVariant &variant);

} // namespace dstools