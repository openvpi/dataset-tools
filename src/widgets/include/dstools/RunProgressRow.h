#pragma once

/// @file RunProgressRow.h
/// @brief Re-exports dsfw::widgets::RunProgressRow into dstools::widgets namespace.

#include <dsfw/widgets/RunProgressRow.h>

namespace dstools::widgets {
using RunProgressRow = dsfw::widgets::RunProgressRow;
}
