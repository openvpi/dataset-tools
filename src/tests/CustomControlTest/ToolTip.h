//
// Created by fluty on 2023/8/30.
//

#ifndef DATASET_TOOLS_TOOLTIP_H
#define DATASET_TOOLS_TOOLTIP_H

#include <QFrame>
#include <QTimer>

#include "WidgetsGlobal/QMWidgetsGlobal.h"

/// Custom Tool Tip
class QMWIDGETS_EXPORT ToolTip : public QFrame {
    Q_OBJECT

public:
    enum ToolTipPosition  {
        TOP,
        BOTTOM,
        LEFT,
        RIGHT,
        TOP_LEFT,
        TOP_RIGHT,
        BOTTOM_LEFT,
        BOTTOM_RIGHT
    };

    explicit ToolTip(QString text = "", QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);
    int duration() const;
    void setDuration(const int duration);
    void adjustPos(ToolTipPosition &position);

protected:
    QString m_text;
    int m_duration = 1000; // ms
    QFrame m_container;
    QTimer m_timer;

    void setQss();
};

class ToolTipFilter : public QObject {

public:
    explicit ToolTipFilter(QWidget *parent,
                           int showDelay = 300,
                           const ToolTip::ToolTipPosition &position = ToolTip::BOTTOM_RIGHT);
    ~ToolTipFilter() override;

    bool eventFilter(QObject *object, QEvent *event) override;

protected:
    QTimer m_timer;
    ToolTip *m_tooltip;
    QWidget *m_parent;

    void adjustToolTipPos();
};

#endif // DATASET_TOOLS_TOOLTIP_H
