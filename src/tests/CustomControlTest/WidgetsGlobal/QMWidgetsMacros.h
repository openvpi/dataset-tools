//
// Created by fluty on 2023/8/13.
//

#ifndef DATASET_TOOLS_QMWIDGETSMACROS_H
#define DATASET_TOOLS_QMWIDGETSMACROS_H

#include "QMMacros.h"

// Cast to QWidget
#define Q_W(T) qobject_cast<QWidget *>(T)

#define Q_WARNING QMessageBox::warning
#define Q_ERROR QMessageBox::critical

#include "macros/q_property.h"

#include "macros/q_layout_property.h"

#include "macros/q_d_layout_property.h"

#define Q_FIND_PARENT_WIDGET(ParentClass, FunName) ParentClass *FunName() const;

#define Q_FIND_PARENT_WIDGET_DECLARE(Class, ParentClass, FunName)                                  \
    ParentClass *Class::FunName() const {                                                          \
        static QWidget *ptr = nullptr;                                                             \
        if (!ptr) {                                                                                \
            ptr = parentWidget();                                                                  \
            while (ptr && qstrcmp(ptr->metaObject()->className(),                                  \
                                  ParentClass::staticMetaObject.className())) {                    \
                ptr = ptr->parentWidget();                                                         \
            }                                                                                      \
        }                                                                                          \
        return qobject_cast<ParentClass *>(ptr);                                                   \
    }

#endif // DATASET_TOOLS_QMWIDGETSMACROS_H
