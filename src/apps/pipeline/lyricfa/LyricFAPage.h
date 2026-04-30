#pragma once
#include <dstools/TaskWindow.h>
#include <QCheckBox>
#include <QPushButton>
#include <QSharedPointer>

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

namespace dstools::widgets {
class PathSelector;
class ModelLoadPanel;
}

// Forward declarations for lyricfa types
namespace LyricFA {
class Asr;
class MatchLyric;
}

namespace Pinyin {
class Pinyin;
}

class LyricFAPage : public dstools::widgets::TaskWindow,
                    public dstools::labeler::IPageActions,
                    public dstools::labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)
public:
    explicit LyricFAPage(QWidget *parent = nullptr);
    ~LyricFAPage() override;

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
    void slot_matchLyric();
    void slot_loadModel();

private:
    dstools::widgets::PathSelector *m_labPath;
    dstools::widgets::PathSelector *m_jsonPath;
    dstools::widgets::PathSelector *m_lyricPath;
    QCheckBox *m_pinyinBox;
    QPushButton *m_matchBtn;

    dstools::widgets::ModelLoadPanel *m_modelPanel;

    LyricFA::Asr *m_asr = nullptr;
    QSharedPointer<Pinyin::Pinyin> m_mandarin;
    LyricFA::MatchLyric *m_match = nullptr;

    enum Mode { Mode_Asr, Mode_MatchLyric };
    Mode m_currentMode = Mode_Asr;
    QString m_workingDir;
};
