#pragma once

/// @file FileProgressTracker.h
/// @brief Re-exports dsfw::widgets::FileProgressTracker into dstools::widgets namespace.

#include <dsfw/widgets/FileProgressTracker.h>

namespace dstools::widgets {
using FileProgressTracker = dsfw::widgets::FileProgressTracker;
}
