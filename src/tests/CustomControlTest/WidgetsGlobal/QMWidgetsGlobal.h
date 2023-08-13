//
// Created by fluty on 2023/8/13.
//

#ifndef DATASET_TOOLS_QMWIDGETSGLOBAL_H
#define DATASET_TOOLS_QMWIDGETSGLOBAL_H

#include <QtGlobal>

#include "QMGuiGlobal.h"
#include "QMWidgetsMacros.h"

#ifndef QMWIDGETS_EXPORT
#    ifdef QMWIDGETS_STATIC
#        define QMWIDGETS_EXPORT
#    else
#        ifdef QMWIDGETS_LIBRARY
#            define QMWIDGETS_EXPORT Q_DECL_EXPORT
#        else
#            define QMWIDGETS_EXPORT //Q_DECL_IMPORT
#        endif
#    endif
#endif

#endif // DATASET_TOOLS_QMWIDGETSGLOBAL_H
