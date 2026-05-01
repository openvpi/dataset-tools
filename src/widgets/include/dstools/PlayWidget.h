#pragma once

/// @file PlayWidget.h
/// @brief Re-exports dsfw::widgets::PlayWidget into dstools::widgets namespace.

#include <dsfw/widgets/PlayWidget.h>

namespace dstools::widgets {
using PlayWidget = dsfw::widgets::PlayWidget;
}
