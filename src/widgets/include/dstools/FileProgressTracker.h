#pragma once
#include <dstools/WidgetsGlobal.h>
#include <QWidget>
#include <functional>

class QLabel;
class QProgressBar;

namespace dstools::widgets {

/// A compact progress display widget for file-based workflows.
/// Shows "X / Y (Z%)" with optional QProgressBar.
class DSTOOLS_WIDGETS_API FileProgressTracker : public QWidget {
    Q_OBJECT
public:
    enum DisplayStyle { LabelOnly, ProgressBarStyle };

    explicit FileProgressTracker(DisplayStyle style = LabelOnly, QWidget *parent = nullptr);
    ~FileProgressTracker() override;

    /// Update progress display.
    void setProgress(int completed, int total);

    /// Set display style.
    void setDisplayStyle(DisplayStyle style);

    /// Set the progress text format. Use %1=completed, %2=total, %3=percent.
    /// Default: "%1 / %2 (%3%)"
    void setFormat(const QString &format);

    /// Set the text shown when total is 0.
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

} // namespace dstools::widgets
