//
// Created by fluty on 2023/8/16.
//

#include <QGraphicsDropShadowEffect>

#include "ShadowButton.h"

ShadowButton::ShadowButton(QWidget *parent) : QPushButton(parent) {
    auto applyShadow = [](QWidget *widget, QWidget *parent) {
        auto *shadow = new QGraphicsDropShadowEffect(parent);
        shadow->setOffset(0, 2.5);
        shadow->setColor(QColor(0, 0, 0, 15));
        shadow->setBlurRadius(7.5);
        widget->setGraphicsEffect(shadow);
    };

    applyShadow(this, parent);
}
ShadowButton::~ShadowButton() {
}
