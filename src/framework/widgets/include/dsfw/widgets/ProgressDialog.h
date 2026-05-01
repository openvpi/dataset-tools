#pragma once
/// @file ProgressDialog.h
/// @brief Modal progress dialog with label, progress bar, and cancel button.

#include <dsfw/widgets/WidgetsGlobal.h>

#include <QDialog>

class QProgressBar;
class QLabel;
class QPushButton;

namespace dsfw::widgets {

/// @brief Modal progress dialog with label, progress bar, and optional cancel button.
class DSFW_WIDGETS_API ProgressDialog : public QDialog {
    Q_OBJECT
public:
    /// @brief Construct a ProgressDialog.
    /// @param title Dialog window title.
    /// @param parent Parent widget.
    explicit ProgressDialog(const QString &title = {}, QWidget *parent = nullptr);

    /// @brief Set the progress range.
    /// @param minimum Minimum value.
    /// @param maximum Maximum value.
    void setRange(int minimum, int maximum);

    /// @brief Set the current progress value.
    /// @param value Current value.
    void setValue(int value);

    /// @brief Get the current progress value.
    /// @return Current value.
    int value() const;

    /// @brief Set the descriptive label text.
    /// @param text Label text.
    void setLabelText(const QString &text);

    /// @brief Get the current label text.
    /// @return Label text.
    QString labelText() const;

    /// @brief Enable or disable the cancel button.
    /// @param enabled True to show the cancel button.
    void setCancelButtonEnabled(bool enabled);

signals:
    /// @brief Emitted when the cancel button is clicked.
    void canceled();

private:
    QProgressBar *m_progressBar;
    QLabel *m_label;
    QPushButton *m_cancelBtn;
    int m_value = 0;
};

} // namespace dsfw::widgets
