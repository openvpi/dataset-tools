#pragma once

#include <QStringList>
#include <dsfw/TaskTypes.h>
#include <dstools/TimePos.h>

namespace dstools {

    LayerData buildBoundaries(const QStringList &items, const QStringList &durs, TimePos initialPos);

} // namespace dstools