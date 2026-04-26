#pragma once
#include <dstools/TaskWindow.h>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSharedPointer>

// Forward declarations for lyricfa types
namespace LyricFA {
class Asr;
class MatchLyric;
}

namespace Pinyin {
class Pinyin;
}

class LyricFAPage : public dstools::widgets::TaskWindow {
    Q_OBJECT
public:
    explicit LyricFAPage(QWidget *parent = nullptr);
    ~LyricFAPage() override;

protected:
    void init() override;
    void runTask() override;
    void onTaskFinished() override;

private slots:
    void slot_matchLyric();
    void slot_labPath();
    void slot_jsonPath();
    void slot_lyricPath();
    void slot_browseModel();
    void slot_loadModel();

private:
    QLineEdit *m_labEdit;
    QLineEdit *m_jsonEdit;
    QLineEdit *m_lyricEdit;
    QCheckBox *m_pinyinBox;
    QPushButton *m_matchBtn;

    QLineEdit *m_modelEdit;
    QLabel *m_modelStatusLabel;

    LyricFA::Asr *m_asr = nullptr;
    QSharedPointer<Pinyin::Pinyin> m_mandarin;
    LyricFA::MatchLyric *m_match = nullptr;

    enum Mode { Mode_Asr, Mode_MatchLyric };
    Mode m_currentMode = Mode_Asr;
};
