//
// Created by fluty on 2023/8/13.
//

#ifndef DATASET_TOOLS_EDITLABEL_H
#define DATASET_TOOLS_EDITLABEL_H

#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>

class EditLabel : public QStackedWidget {
    Q_OBJECT

public:
    explicit EditLabel(QWidget *parent = nullptr);
    // explicit EditLabel(const QString &text, QWidget *parent = nullptr);

    QLabel *label;
    QLineEdit *lineEdit;

    QString text() const;
    void setText(const QString &text);

signals:
    void valueChanged(const QString &text);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void initUi(const QString &text);
};

#endif // DATASET_TOOLS_EDITLABEL_H
