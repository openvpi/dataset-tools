#pragma once

/// @file ViewportController.h
/// @brief Re-exports dsfw::widgets::ViewportState and ViewportController into dstools::widgets namespace.

#include <dsfw/widgets/ViewportController.h>

namespace dstools::widgets {
using ViewportState = dsfw::widgets::ViewportState;
using ViewportController = dsfw::widgets::ViewportController;
}
