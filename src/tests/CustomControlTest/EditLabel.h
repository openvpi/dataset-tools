//
// Created by fluty on 2023/8/13.
//

#ifndef DATASET_TOOLS_EDITLABEL_H
#define DATASET_TOOLS_EDITLABEL_H

#include "QWidget"
#include "WidgetsGlobal/QMWidgetsGlobal.h"
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>

// class EditLabelPrivate;

class QMWIDGETS_EXPORT EditLabel : public QStackedWidget {
    Q_OBJECT

public:
    explicit EditLabel(QWidget *parent = nullptr);
    explicit EditLabel(const QString &text, QWidget *parent = nullptr);
    ~EditLabel();

    QLabel *label;
    QLineEdit *lineEdit;

    QString text() const;
    void setText(const QString &text);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void initUi(const QString &text);

private:
};

#endif // DATASET_TOOLS_EDITLABEL_H
