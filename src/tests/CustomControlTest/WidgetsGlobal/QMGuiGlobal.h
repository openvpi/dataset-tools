//
// Created by fluty on 2023/8/13.
//

#ifndef DATASET_TOOLS_QMGUIGLOBAL_H
#define DATASET_TOOLS_QMGUIGLOBAL_H

#include <QtGlobal>

#include "QMGlobal.h"

#ifndef QMGUI_EXPORT
#    ifdef QMGUI_STATIC
#        define QMGUI_EXPORT
#    else
#        ifdef QMGUI_LIBRARY
#            define QMGUI_EXPORT Q_DECL_EXPORT
#        else
#            define QMGUI_EXPORT Q_DECL_IMPORT
#        endif
#    endif
#endif

#endif // DATASET_TOOLS_QMGUIGLOBAL_H
