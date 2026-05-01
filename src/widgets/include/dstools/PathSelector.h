#pragma once

/// @file PathSelector.h
/// @brief Re-exports dsfw::widgets::PathSelector into dstools::widgets namespace.

#include <dsfw/widgets/PathSelector.h>

namespace dstools::widgets {
using PathSelector = dsfw::widgets::PathSelector;
}
