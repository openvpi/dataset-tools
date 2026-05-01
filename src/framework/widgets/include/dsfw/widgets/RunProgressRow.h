#pragma once
/// @file RunProgressRow.h
/// @brief Horizontal widget row with a run/cancel button and progress bar.

#include <dsfw/widgets/WidgetsGlobal.h>
#include <QWidget>

class QPushButton;
class QProgressBar;

namespace dsfw::widgets {

/// @brief Horizontal row with a run/cancel button and progress bar.
class DSFW_WIDGETS_API RunProgressRow : public QWidget {
    Q_OBJECT
public:
    /// @brief Construct a RunProgressRow.
    /// @param buttonText Label for the run button.
    /// @param parent Parent widget.
    explicit RunProgressRow(const QString &buttonText = QStringLiteral("Run"),
                            QWidget *parent = nullptr);

public slots:
    /// @brief Set the progress bar value.
    /// @param percent Progress percentage (0-100).
    void setProgress(int percent);

    /// @brief Toggle between run and cancel button states.
    /// @param running True to show cancel, false to show run.
    void setRunning(bool running);

    /// @brief Enable or disable the button.
    /// @param enabled True to enable.
    void setButtonEnabled(bool enabled);

    /// @brief Reset the progress bar and button to initial state.
    void reset();

signals:
    /// @brief Emitted when the run button is clicked.
    void runClicked();

    /// @brief Emitted when the cancel button is clicked.
    void cancelClicked();

private:
    QPushButton *m_button;
    QProgressBar *m_progressBar;
    QString m_runText;
    bool m_running = false;
};

} // namespace dsfw::widgets
