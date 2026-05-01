#pragma once

#include <QtGlobal>

#ifdef DSFW_WIDGETS_BUILD
#define DSFW_WIDGETS_API Q_DECL_EXPORT
#else
#define DSFW_WIDGETS_API Q_DECL_IMPORT
#endif
