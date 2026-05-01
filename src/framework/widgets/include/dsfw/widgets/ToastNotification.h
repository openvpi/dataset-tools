#pragma once

/// @file ToastNotification.h
/// @brief Non-modal floating notification widget with auto-dismiss.

#include <dsfw/widgets/WidgetsGlobal.h>

#include <QFrame>
#include <QString>

class QLabel;
class QTimer;
class QPropertyAnimation;

namespace dsfw::widgets {

/// @brief Notification severity level.
enum class ToastType {
    Info,    ///< Informational message.
    Warning, ///< Warning message.
    Error    ///< Error message.
};

/// @brief Non-modal floating notification that auto-dismisses after a timeout.
///
/// Displays at the bottom of the parent widget, slides in with animation,
/// and fades out when the timer expires. Multiple toasts stack vertically.
///
/// @code
/// ToastNotification::show(parentWidget, ToastType::Info, "File saved.", 3000);
/// @endcode
class DSFW_WIDGETS_API ToastNotification : public QFrame {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
    /// @brief Show a toast notification on the given parent widget.
    /// @param parent Parent widget (toast is positioned relative to this).
    /// @param type Notification type (Info/Warning/Error).
    /// @param message Message text.
    /// @param timeoutMs Auto-dismiss timeout in milliseconds (default: 3000).
    static void show(QWidget *parent, ToastType type, const QString &message,
                     int timeoutMs = 3000);

    ~ToastNotification() override;

    /// @brief Current opacity value.
    /// @return Opacity in [0.0, 1.0].
    qreal opacity() const;

    /// @brief Set the opacity for fade-out animation.
    /// @param opacity Opacity in [0.0, 1.0].
    void setOpacity(qreal opacity);

protected:
    /// @brief Custom paint to apply opacity.
    void paintEvent(QPaintEvent *event) override;

    /// @brief Intercept parent resize to reposition.
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    explicit ToastNotification(QWidget *parent, ToastType type,
                               const QString &message, int timeoutMs);
    void startDismiss();
    void reposition();

    QLabel *m_label = nullptr;
    QTimer *m_timer = nullptr;
    ToastType m_type;
    qreal m_opacity = 1.0;
};

} // namespace dsfw::widgets
