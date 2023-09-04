//
// Created by fluty on 2023/8/30.
//

#ifndef DATASET_TOOLS_TOOLTIP_H
#define DATASET_TOOLS_TOOLTIP_H

#include <QFrame>
#include <QPropertyAnimation>
#include <QTimer>

#include "WidgetsGlobal/QMWidgetsGlobal.h"

/// Custom Tool Tip
class QMWIDGETS_EXPORT ToolTip : public QFrame {
    Q_OBJECT

public:
//    enum ToolTipPosition  {
//        TOP,
//        BOTTOM,
//        LEFT,
//        RIGHT,
//        TOP_LEFT,
//        TOP_RIGHT,
//        BOTTOM_LEFT,
//        BOTTOM_RIGHT
//    };

    explicit ToolTip(QString text = "", QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);
    int duration() const;
    void setDuration(const int duration);

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
                           int showDelay = 1000,
                           bool followCursor = false,
                           bool animation = true);
    ~ToolTipFilter() override;

    int showDelay();
    void setShowDelay(int delay);
    bool followCursor();
    void setFollowCursor(bool on);
    bool animation();
    void setAnimation(bool on);

protected:
    QTimer m_timer;
    ToolTip *m_tooltip;
    QWidget *m_parent;
    int m_showDelay;
    bool m_followCursor;
    bool m_animation;
    bool mouseInParent = false;

    // Animation
    QPropertyAnimation *m_opacityAnimation;

    void adjustToolTipPos();
    void showToolTip();
    void hideToolTip();

    bool eventFilter(QObject *object, QEvent *event) override;
};

#endif // DATASET_TOOLS_TOOLTIP_H
