#pragma once
/// @file WidgetsGlobal.h
/// @brief Shared export/import macro for the dsfw-widgets library.

#include <QtGlobal>

#ifdef DSFW_WIDGETS_BUILD
#define DSFW_WIDGETS_API Q_DECL_EXPORT
#else
#define DSFW_WIDGETS_API Q_DECL_IMPORT
#endif
