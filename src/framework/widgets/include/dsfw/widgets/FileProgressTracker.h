#pragma once
/// @file FileProgressTracker.h
/// @brief Widget displaying file processing progress as a label or progress bar.

#include <dsfw/widgets/WidgetsGlobal.h>
#include <QWidget>
#include <functional>

class QLabel;
class QProgressBar;

namespace dsfw::widgets {

/// @brief Widget showing file processing progress as label or progress bar.
class DSFW_WIDGETS_API FileProgressTracker : public QWidget {
    Q_OBJECT
public:
    /// @brief Display style for the progress tracker.
    enum DisplayStyle { LabelOnly, ProgressBarStyle };

    /// @brief Construct a FileProgressTracker.
    /// @param style Display style.
    /// @param parent Parent widget.
    explicit FileProgressTracker(DisplayStyle style = LabelOnly, QWidget *parent = nullptr);
    ~FileProgressTracker() override;

    /// @brief Update the progress values.
    /// @param completed Number of completed files.
    /// @param total Total number of files.
    void setProgress(int completed, int total);

    /// @brief Change the display style.
    /// @param style New display style.
    void setDisplayStyle(DisplayStyle style);

    /// @brief Set a custom format string for the progress text.
    /// @param format Format string (e.g. "%1 / %2 files").
    void setFormat(const QString &format);

    /// @brief Set the text shown when there are no files.
    /// @param text Empty state text.
    void setEmptyText(const QString &text);

private:
    void buildLayout();
    void updateDisplay();

    DisplayStyle m_style;
    QString m_format;
    QString m_emptyText;
    int m_completed = 0;
    int m_total = 0;

    QLabel *m_label = nullptr;
    QProgressBar *m_progressBar = nullptr;
};

} // namespace dsfw::widgets
