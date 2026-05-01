#pragma once

/// @file TaskWindow.h
/// @brief Re-exports dsfw::widgets::TaskWindow into dstools::widgets namespace.

#include <dsfw/widgets/TaskWindow.h>

namespace dstools::widgets {
using TaskWindow = dsfw::widgets::TaskWindow;
}
