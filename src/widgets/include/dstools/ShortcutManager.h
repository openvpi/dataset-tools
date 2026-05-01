#pragma once

/// @file ShortcutManager.h
/// @brief Re-exports dsfw::widgets::ShortcutManager into dstools::widgets namespace.

#include <dsfw/widgets/ShortcutManager.h>

namespace dstools::widgets {
using ShortcutManager = dsfw::widgets::ShortcutManager;
}
