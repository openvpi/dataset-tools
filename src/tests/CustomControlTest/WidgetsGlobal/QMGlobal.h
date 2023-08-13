//
// Created by fluty on 2023/8/13.
//

#ifndef DATASET_TOOLS_QMGLOBAL_H
#define DATASET_TOOLS_QMGLOBAL_H

#include <QtGlobal>

#include "QMMacros.h"

#ifndef QMCORE_EXPORT
#  ifdef QMCORE_STATIC
#    define QMCORE_EXPORT
#  else
#    ifdef QMCORE_LIBRARY
#      define QMCORE_EXPORT Q_DECL_EXPORT
#    else
#      define QMCORE_EXPORT Q_DECL_IMPORT
#    endif
#  endif
#endif

#endif // DATASET_TOOLS_QMGLOBAL_H
