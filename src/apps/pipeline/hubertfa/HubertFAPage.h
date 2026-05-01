#pragma once
#include <dstools/TaskWindow.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QTextCharFormat>

#include <dsfw/IAlignmentService.h>
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dstools/PathSelector.h>

namespace dstools::widgets {

class ModelLoadPanel;
}

class HubertFAPage : public dstools::widgets::TaskWindow,
                     public dstools::labeler::IPageActions,
                     public dstools::labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)
public:
    explicit HubertFAPage(QWidget *parent = nullptr);
    ~HubertFAPage() override;

    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

    void onWorkingDirectoryChanged(const QString &newDir) override;

protected:
    void init() override;
    void runTask() override;
    void onTaskFinished() override;

private slots:
    void slot_loadModel();
    void slot_hfaFailed(const QString &filename, const QString &msg);
    void slot_hfaFinished(const QString &filename, const QString &msg);

private:
    dstools::widgets::PathSelector *m_outTgPath;
    dstools::widgets::ModelLoadPanel *m_modelPanel;
    QButtonGroup *m_languageGroup = nullptr;
    QHBoxLayout *m_nonSpeechPhLayout = nullptr;
    QWidget *m_dynamicContainer = nullptr;
    dstools::IAlignmentService *m_alignmentService = nullptr;
    QTextCharFormat m_errorFormat;
    QString m_workingDir;
};
