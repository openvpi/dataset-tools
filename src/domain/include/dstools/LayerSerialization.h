#pragma once

#include <dsfw/TaskTypes.h>
#include <dstools/LayerTypes.h>
#include <dsfw/Result.h>

#include <QString>

namespace dstools {

    dsfw::Result<LayerDataVariant> parseLayerData(const dsfw::LayerData &data, const QString &layerType);

    dsfw::LayerData serializeLayerData(const LayerDataVariant &variant);

} // namespace dstools