//
// Created by fluty on 2023/8/13.
//

#include "EditLabel.h"
#include "QDebug"
#include "QEvent"
#include "QKeyEvent"
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>

EditLabel::~EditLabel() {
}

EditLabel::EditLabel(QWidget *parent) : QStackedWidget(parent) {
    initUi("EditLabel");
}

EditLabel::EditLabel(const QString &text, QWidget *parent) : QStackedWidget(parent) {
    initUi(text);
}

void EditLabel::initUi(const QString &text) {
    this->setAttribute(Qt::WA_StyledBackground);

    label = new QLabel;
    label->setText(text);
    label->setFocusPolicy(Qt::FocusPolicy::ClickFocus);
    label->installEventFilter(this);

    lineEdit = new QLineEdit;
    lineEdit->installEventFilter(this);
    lineEdit->setContextMenuPolicy(Qt::NoContextMenu);
    lineEdit->setStyleSheet(QString("border: 1px solid #d4d4d4;"
                                    "border-bottom: 2px solid #709cff;"
                                    "background-color: #fff; "
                                    "border-radius: 6px; color: #333;"
                                    "selection-color: #FFFFFF;"
                                    "selection-background-color: #709cff;"
                                    "padding: 4px;"));

    this->addWidget(label);
    this->addWidget(lineEdit);
    this->setCurrentWidget(label);

    this->setMaximumHeight(this->sizeHint().height());
}

bool EditLabel::eventFilter(QObject *object, QEvent *event) {
    if (object == label) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            lineEdit->setText(label->text());
            this->setCurrentWidget(lineEdit);
        }
    } else if (object == lineEdit) {
        auto commit = [&]() {
            label->setText(lineEdit->text());
            this->setCurrentWidget(label);
        };
        if (event->type() == QEvent::KeyPress) {
            auto keyEvent = static_cast<QKeyEvent *>(event);
            auto key = keyEvent->key();
            if (key == Qt::Key_Escape || key == Qt::Key_Return) // Esc or Enter pressed
                commit();
        } else if (event->type() == QEvent::FocusOut) {
            //            qDebug() << "FocusOut";
            commit();
        } else if (event->type() == QEvent::ContextMenu) {
            //            qDebug() << "ContextMenu";
        }
    }

    return QWidget::eventFilter(object, event);
}

QString EditLabel::text() const {
    return label->text();
}

void EditLabel::setText(const QString &text) {
    label->setText(text);
}