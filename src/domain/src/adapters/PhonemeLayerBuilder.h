#pragma once

#include <QStringList>
#include <dsfw/TaskTypes.h>
#include <dsfw/TimePos.h>

namespace dstools {

dsfw::LayerData buildBoundaries(const QStringList& items, const QStringList& durs, dsfw::TimePos initialPos);

}  // namespace dstools