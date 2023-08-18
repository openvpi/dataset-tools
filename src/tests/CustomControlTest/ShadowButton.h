//
// Created by fluty on 2023/8/16.
//

#ifndef DATASET_TOOLS_SHADOWBUTTON_H
#define DATASET_TOOLS_SHADOWBUTTON_H

#include <QPushButton>

#include "WidgetsGlobal/QMWidgetsGlobal.h"

class QMWIDGETS_EXPORT ShadowButton : public QPushButton {
    Q_OBJECT

public:
    explicit ShadowButton(QWidget *parent);
    ~ShadowButton();
};



#endif // DATASET_TOOLS_SHADOWBUTTON_H
