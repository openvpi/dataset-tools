#pragma once
#include <dstools/TaskWindow.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QTextCharFormat>

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

namespace dstools::widgets {
class PathSelector;
class ModelLoadPanel;
}

namespace HFA {
class HFA;
}

class HubertFAPage : public dstools::widgets::TaskWindow,
                     public dstools::labeler::IPageActions,
                     public dstools::labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)
public:
    explicit HubertFAPage(QWidget *parent = nullptr);
    ~HubertFAPage() override;

    // IPageActions
    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

    // IPageLifecycle
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
    HFA::HFA *m_hfa = nullptr;
    QTextCharFormat m_errorFormat;
    QString m_workingDir;
};
