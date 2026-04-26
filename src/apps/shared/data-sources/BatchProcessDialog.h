#pragma once

#include <QDialog>
#include <QStringList>
#include <utility>

class QCheckBox;
class QComboBox;
class QLabel;
class QListWidget;
class QTextEdit;
class QProgressBar;
class QPushButton;
class QVBoxLayout;
class QToolButton;

namespace dstools {

    class BatchProcessDialog : public QDialog {
        Q_OBJECT

    public:
        enum class LogLevel { Info, Warning, Error };

        explicit BatchProcessDialog(const QString &title, QWidget *parent = nullptr);
        ~BatchProcessDialog() override;

        void addParamWidget(QWidget *widget);
        void addParamRow(const QString &label, QWidget *widget);
        void addParamGroup(const QString &title);

        void appendLog(const QString &message);
        void appendLog(LogLevel level, const QString &message);
        void setProgress(int current, int total);
        void finish(int processed, int skipped, int errors);

        /// Record a per-item error for the summary panel.
        void recordError(const QString &sliceId, const QString &errorMessage);

        /// Show error summary panel after batch completion.
        void showErrorSummary();

        /// Returns the list of recorded errors as (sliceId, error) pairs.
        [[nodiscard]] const std::vector<std::pair<QString, QString>> &errorItems() const;

        /// Update progress during cancellation phase (items waiting for current work to finish).
        /// @param remaining Number of items still being cleaned up.
        /// @param total Total items being cancelled.
        void setCancelProgress(int remaining, int total);

        [[nodiscard]] bool isCancelled() const;

    signals:
        void started();
        void cancelled();

    private:
        void onCancelClicked();
        void reject() override;
        void toggleParamGroup(QToolButton *toggleBtn, QWidget *content);

        QTextEdit *m_logOutput = nullptr;
        QProgressBar *m_progressBar = nullptr;
        QPushButton *m_startBtn = nullptr;
        QPushButton *m_cancelBtn = nullptr;
        QPushButton *m_closeBtn = nullptr;
        QLabel *m_statusLabel = nullptr;

        QVBoxLayout *m_paramLayout = nullptr;

        QListWidget *m_errorList = nullptr;
        QPushButton *m_exportErrorBtn = nullptr;
        std::vector<std::pair<QString, QString>> m_errorItems;

        bool m_running = false;
        bool m_cancelled = false;
    };

} // namespace dstools
