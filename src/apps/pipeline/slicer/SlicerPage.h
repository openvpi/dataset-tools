#pragma once
#include <dstools/TaskWindow.h>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>

namespace dstools::widgets {
class PathSelector;
}

class SlicerPage : public dstools::widgets::TaskWindow {
    Q_OBJECT
public:
    explicit SlicerPage(QWidget *parent = nullptr);
    ~SlicerPage() override;

protected:
    void init() override;
    void runTask() override;

private slots:
    void onOneFinished(const QString &filename, int listIndex);
    void onOneFailed(const QString &errmsg, int listIndex);

private:
    void logMessage(const QString &txt);
    void addSingleAudioFile(const QString &fullPath);

    QLineEdit *m_lineThreshold;
    QLineEdit *m_lineMinLength;
    QLineEdit *m_lineMinInterval;
    QLineEdit *m_lineHopSize;
    QLineEdit *m_lineMaxSilence;
    QComboBox *m_cmbOutputFormat;
    QComboBox *m_cmbSlicingMode;
    QSpinBox *m_spnSuffixDigits;
    QCheckBox *m_chkOverwriteMarkers;
    dstools::widgets::PathSelector *m_outputDir;

    int m_workTotal = 0;
    int m_workFinished = 0;
    int m_workError = 0;
    QStringList m_failIndex;
};
