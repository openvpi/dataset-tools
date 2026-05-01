/// @file HubertFAPage.h
/// @brief HuBERT forced alignment pipeline page.

#pragma once
#include <dstools/TaskWindow.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QTextCharFormat>

#include <dsfw/ITaskProcessor.h>
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/BatchCheckpoint.h>
#include <dstools/PathSelector.h>

namespace dstools::widgets {

class ModelLoadPanel;
}

/// @brief TaskWindow+IPageActions+IPageLifecycle page for batch HuBERT-FA
/// alignment with model loading and language selection.
class HubertFAPage : public dstools::widgets::TaskWindow,
                     public dstools::labeler::IPageActions,
                     public dstools::labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)
public:
    /// @param parent Optional parent widget.
    explicit HubertFAPage(QWidget *parent = nullptr);
    ~HubertFAPage() override;

    /// @brief Set the working directory for alignment input.
    void setWorkingDirectory(const QString &dir) override;
    /// @brief Get the current working directory.
    QString workingDirectory() const override;

    /// @brief Handle working directory changes.
    void onWorkingDirectoryChanged(const QString &newDir) override;

protected:
    /// @brief Initialize UI elements.
    void init() override;
    /// @brief Run the HuBERT-FA alignment task.
    void runTask() override;
    /// @brief Handle task completion.
    void onTaskFinished() override;

private slots:
    void slot_loadModel();
    void slot_hfaFailed(const QString &filename, const QString &msg);
    void slot_hfaFinished(const QString &filename, const QString &msg);

private:
    dstools::widgets::PathSelector *m_outTgPath;       ///< TextGrid output path selector.
    dstools::widgets::ModelLoadPanel *m_modelPanel;     ///< Model loading panel.
    QButtonGroup *m_languageGroup = nullptr;            ///< Language selection radio group.
    QHBoxLayout *m_nonSpeechPhLayout = nullptr;         ///< Non-speech phoneme layout.
    QWidget *m_dynamicContainer = nullptr;              ///< Dynamic UI container.
    QCheckBox *m_chkResume = nullptr;
    std::unique_ptr<dstools::ITaskProcessor> m_processor;
    bool m_modelLoaded = false;
    QTextCharFormat m_errorFormat;
    QString m_workingDir;
    dstools::BatchCheckpoint m_checkpoint;

    void updateResumeCheckbox();
};
