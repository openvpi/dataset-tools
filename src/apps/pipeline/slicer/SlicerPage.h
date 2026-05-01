/// @file SlicerPage.h
/// @brief Audio slicer pipeline page with batch processing.

#pragma once
#include <dstools/TaskWindow.h>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/BatchCheckpoint.h>
#include <dstools/PathSelector.h>

namespace dstools::widgets {

}

/// @brief TaskWindow+IPageActions+IPageLifecycle page for batch audio slicing.
class SlicerPage : public dstools::widgets::TaskWindow,
                   public dstools::labeler::IPageActions,
                   public dstools::labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)
public:
    /// @param parent Optional parent widget.
    explicit SlicerPage(QWidget *parent = nullptr);
    ~SlicerPage() override;

    // IPageActions
    /// @brief Set the working directory for audio input.
    void setWorkingDirectory(const QString &dir) override;
    /// @brief Get the current working directory.
    QString workingDirectory() const override;

    // IPageLifecycle
    /// @brief Handle working directory changes.
    void onWorkingDirectoryChanged(const QString &newDir) override;

protected:
    /// @brief Initialize UI elements.
    void init() override;
    /// @brief Run the batch slicing task.
    void runTask() override;

private slots:
    void onOneFinished(const QString &identifier, const QString &msg);
    void onOneFailed(const QString &identifier, const QString &msg);

private:
    void logMessage(const QString &txt);
    void addSingleAudioFile(const QString &fullPath);

    QLineEdit *m_lineThreshold;      ///< RMS threshold input.
    QLineEdit *m_lineMinLength;      ///< Minimum chunk length input.
    QLineEdit *m_lineMinInterval;    ///< Minimum silence interval input.
    QLineEdit *m_lineHopSize;        ///< Hop size input.
    QLineEdit *m_lineMaxSilence;     ///< Maximum kept silence input.
    QComboBox *m_cmbOutputFormat;    ///< Output WAV format selector.
    QComboBox *m_cmbSlicingMode;     ///< Slicing mode selector.
    QSpinBox *m_spnSuffixDigits;     ///< Suffix digit count spin box.
    QCheckBox *m_chkOverwriteMarkers;
    QCheckBox *m_chkResume = nullptr;
    dstools::widgets::PathSelector *m_outputDir;

    int m_workTotal = 0;
    int m_workFinished = 0;
    int m_workError = 0;
    QStringList m_failIndex;
    QString m_workingDir;
    dstools::BatchCheckpoint m_checkpoint;

    void updateResumeCheckbox();
};
