#pragma once

/// @file WidgetsGlobal.h
/// @brief DLL export/import macro definitions for the dstools-widgets library.

#include <QtGlobal>

#ifdef DSTOOLS_WIDGETS_BUILD
#define DSTOOLS_WIDGETS_API Q_DECL_EXPORT ///< DLL export/import macro for dstools-widgets
#else
#define DSTOOLS_WIDGETS_API Q_DECL_IMPORT ///< DLL export/import macro for dstools-widgets
#endif
