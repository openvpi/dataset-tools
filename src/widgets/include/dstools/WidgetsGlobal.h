#pragma once
#include <QtGlobal>

#ifdef DSTOOLS_WIDGETS_BUILD
#define DSTOOLS_WIDGETS_API Q_DECL_EXPORT
#else
#define DSTOOLS_WIDGETS_API Q_DECL_IMPORT
#endif
