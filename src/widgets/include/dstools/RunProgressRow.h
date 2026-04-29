#pragma once
#include <dstools/WidgetsGlobal.h>
#include <QWidget>

class QPushButton;
class QProgressBar;

namespace dstools::widgets {

/// Composite widget: Run/Cancel button + progress bar for long-running tasks.
class DSTOOLS_WIDGETS_API RunProgressRow : public QWidget {
    Q_OBJECT
public:
    explicit RunProgressRow(const QString &buttonText = QStringLiteral("Run"),
                            QWidget *parent = nullptr);

public slots:
    void setProgress(int percent);
    void setRunning(bool running);
    void setButtonEnabled(bool enabled);
    void reset();

signals:
    void runClicked();
    void cancelClicked();

private:
    QPushButton *m_button;
    QProgressBar *m_progressBar;
    QString m_runText;
    bool m_running = false;
};

} // namespace dstools::widgets
