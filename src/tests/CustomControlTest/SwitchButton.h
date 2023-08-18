//
// Created by fluty on 2023/8/17.
//

#ifndef DATASET_TOOLS_SWITCHBUTTON_H
#define DATASET_TOOLS_SWITCHBUTTON_H

#include "WidgetsGlobal/QMWidgetsGlobal.h"
#include <QPropertyAnimation>
#include <QPushButton>

class QMWIDGETS_EXPORT SwitchButton : public QPushButton{
    Q_OBJECT
    Q_PROPERTY(bool value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double apparentValue READ apparentValue WRITE setApparentValue)

public:
    explicit SwitchButton(QWidget *parent = nullptr);
    explicit SwitchButton(bool on, QWidget *parent = nullptr);
    ~SwitchButton();

    bool value() const;

public slots:
    void setValue(bool value);

signals:
    void valueChanged(bool value);

protected:
    bool m_value;

    QRect m_rect;
//    int m_penWidth;
    int m_thumbRadius;
    int m_halfRectHeight;
    QPoint m_trackStart;
    QPoint m_trackEnd;
    int m_trackLength;
    QPropertyAnimation *m_valueAnimation;

    void initUi(QWidget *parent);
    void calculateParams();
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    int m_apparentValue = 0;
    int apparentValue() const;
    void setApparentValue(int x);
};



#endif // DATASET_TOOLS_SWITCHBUTTON_H
