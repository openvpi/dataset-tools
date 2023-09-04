//
// Created by fluty on 2023/8/30.
//

#ifndef DATASET_TOOLS_TOOLTIP_H
#define DATASET_TOOLS_TOOLTIP_H

#include <QFrame>
#include <QPropertyAnimation>
#include <QTimer>
#include <QTextEdit>
#include <QVBoxLayout>

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

    explicit ToolTip(QString title = "", QWidget *parent = nullptr);
    ~ToolTip();

    QString title() const;
    void setTitle(const QString &text);
    QString shortcutKey() const;
    void setShortcutKey(const QString &text);
    QList<QString> message() const;
    void setMessage(const QList<QString> &text);
    void appendMessage(const QString &text);
    void clearMessage();

protected:
    QString m_title;
    QString m_shortcutKey;
    QList<QString> m_message;
    QFrame m_container;
    QTimer m_timer;

    QLabel *m_lbTitle;
    QLabel *m_lbShortcutKey;
    QVBoxLayout *m_cardLayout;
    QVBoxLayout *m_messageLayout;
//    QTextEdit *m_teMessage;

    void setQss();
    void updateMessage();
};

class ToolTipFilter : public QObject {

public:
    explicit ToolTipFilter(QWidget *parent,
                           int showDelay = 1000,
                           bool followCursor = false,
                           bool animation = true);
    ~ToolTipFilter() override;

    int showDelay() const;
    void setShowDelay(int delay);
    bool followCursor() const;
    void setFollowCursor(bool on);
    bool animation() const;
    void setAnimation(bool on);
    QString title() const;
    void setTitle(const QString &text);
    QString shortcutKey() const;
    void setShortcutKey(const QString &text);
    QList<QString> message() const;
    void setMessage(const QList<QString> &text);
    void appendMessage(const QString &text);
    void clearMessage();

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
