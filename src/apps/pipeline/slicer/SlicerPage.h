#pragma once
#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QPushButton>
#include <QThreadPool>

class SlicerPage : public QWidget {
    Q_OBJECT
public:
    explicit SlicerPage(QWidget *parent = nullptr);
    ~SlicerPage() override;

private slots:
    void onBrowseOutputDir();
    void onAddFiles();
    void onAddFolder();
    void onRemoveItem();
    void onClearList();
    void onStart();
    void onOneFinished(const QString &filename, int listIndex);
    void onOneFailed(const QString &errmsg, int listIndex);

private:
    void setupUI();
    void setProcessing(bool processing);
    void logMessage(const QString &txt);
    void addSingleAudioFile(const QString &fullPath);

    QListWidget *m_fileList;
    QLineEdit *m_lineThreshold;
    QLineEdit *m_lineMinLength;
    QLineEdit *m_lineMinInterval;
    QLineEdit *m_lineHopSize;
    QLineEdit *m_lineMaxSilence;
    QComboBox *m_cmbOutputFormat;
    QComboBox *m_cmbSlicingMode;
    QSpinBox *m_spnSuffixDigits;
    QCheckBox *m_chkOverwriteMarkers;
    QLineEdit *m_lineOutputDir;
    QPushButton *m_btnRun;
    QProgressBar *m_progressBar;
    QTextEdit *m_logOutput;
    QThreadPool *m_threadPool;

    bool m_processing = false;
    int m_workTotal = 0;
    int m_workFinished = 0;
    int m_workError = 0;
    QStringList m_failIndex;
};
