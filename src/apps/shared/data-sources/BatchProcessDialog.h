#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QVBoxLayout;

namespace dstools {

class BatchProcessDialog : public QDialog {
    Q_OBJECT

public:
    explicit BatchProcessDialog(const QString &title, QWidget *parent = nullptr);
    ~BatchProcessDialog() override;

    void addParamWidget(QWidget *widget);
    void addParamRow(const QString &label, QWidget *widget);

    void appendLog(const QString &message);
    void setProgress(int current, int total);
    void finish(int processed, int skipped, int errors);

    [[nodiscard]] bool isCancelled() const;

signals:
    void started();
    void cancelled();

private:
    void onCancelClicked();
    void reject() override;

    QPlainTextEdit *m_logOutput = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QPushButton *m_startBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QLabel *m_statusLabel = nullptr;

    QVBoxLayout *m_paramLayout = nullptr;

    bool m_running = false;
    bool m_cancelled = false;
};

} // namespace dstools
