#pragma once

/// @file ShortcutEditorWidget.h
/// @brief Re-exports dsfw::widgets::ShortcutEntry and ShortcutEditorWidget into dstools::widgets namespace.

#include <dsfw/widgets/ShortcutEditorWidget.h>

namespace dstools::widgets {
using ShortcutEntry = dsfw::widgets::ShortcutEntry;
using ShortcutEditorWidget = dsfw::widgets::ShortcutEditorWidget;
}
