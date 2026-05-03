/// @file LyricFAPage.h
/// @brief LyricFA ASR and lyric matching pipeline page.

#pragma once
#include <dstools/TaskWindow.h>
#include <QCheckBox>
#include <QPushButton>
#include <QSharedPointer>

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dstools/PathSelector.h>

#include <memory>

namespace dstools::widgets {

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

/// @brief TaskWindow+IPageActions+IPageLifecycle page for batch ASR recognition
/// and lyric-to-ASR matching.
class LyricFAPage : public dstools::widgets::TaskWindow,
                    public dstools::labeler::IPageActions,
                    public dstools::labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)
public:
    /// @param parent Optional parent widget.
    explicit LyricFAPage(QWidget *parent = nullptr);
    ~LyricFAPage() override;

    // IPageActions
    /// @brief Set the working directory for ASR input.
    void setWorkingDirectory(const QString &dir) override;
    /// @brief Get the current working directory.
    QString workingDirectory() const override;

    // IPageLifecycle
    /// @brief Handle working directory changes.
    void onWorkingDirectoryChanged(const QString &newDir) override;

protected:
    /// @brief Initialize UI elements.
    void init() override;
    /// @brief Run the ASR or lyric matching task.
    void runTask() override;
    /// @brief Handle task completion.
    void onTaskFinished() override;

private slots:
    void slot_matchLyric();
    void slot_loadModel();

private:
    dstools::widgets::PathSelector *m_labPath;   ///< Label output path selector.
    dstools::widgets::PathSelector *m_jsonPath;   ///< JSON output path selector.
    dstools::widgets::PathSelector *m_lyricPath;  ///< Lyric input path selector.
    QCheckBox *m_pinyinBox;                       ///< Pinyin conversion checkbox.
    QPushButton *m_matchBtn;                      ///< Lyric match button.

    dstools::widgets::ModelLoadPanel *m_modelPanel; ///< Model loading panel.

    std::unique_ptr<LyricFA::Asr> m_asr;                ///< ASR engine instance.
    QSharedPointer<Pinyin::Pinyin> m_mandarin;    ///< Mandarin pinyin converter.
    std::unique_ptr<LyricFA::MatchLyric> m_match;       ///< Lyric matching engine.

    /// @brief Operating mode of the page.
    enum Mode { Mode_Asr, Mode_MatchLyric };
    Mode m_currentMode = Mode_Asr; ///< Current operating mode.
    QString m_workingDir;          ///< Current working directory.
};
