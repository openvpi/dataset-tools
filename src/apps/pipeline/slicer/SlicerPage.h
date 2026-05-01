#pragma once
#include <dstools/TaskWindow.h>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dstools/PathSelector.h>

namespace dstools::widgets {

}

class SlicerPage : public dstools::widgets::TaskWindow,
                   public dstools::labeler::IPageActions,
                   public dstools::labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)
public:
    explicit SlicerPage(QWidget *parent = nullptr);
    ~SlicerPage() override;

    // IPageActions
    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

    // IPageLifecycle
    void onWorkingDirectoryChanged(const QString &newDir) override;

protected:
    void init() override;
    void runTask() override;

private slots:
    void onOneFinished(const QString &identifier, const QString &msg);
    void onOneFailed(const QString &identifier, const QString &msg);

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
    QString m_workingDir;
};
