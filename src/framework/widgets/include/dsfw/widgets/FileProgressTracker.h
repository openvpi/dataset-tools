#pragma once
#include <dsfw/widgets/WidgetsGlobal.h>
#include <QWidget>
#include <functional>

class QLabel;
class QProgressBar;

namespace dsfw::widgets {

class DSFW_WIDGETS_API FileProgressTracker : public QWidget {
    Q_OBJECT
public:
    enum DisplayStyle { LabelOnly, ProgressBarStyle };

    explicit FileProgressTracker(DisplayStyle style = LabelOnly, QWidget *parent = nullptr);
    ~FileProgressTracker() override;

    void setProgress(int completed, int total);
    void setDisplayStyle(DisplayStyle style);
    void setFormat(const QString &format);
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
