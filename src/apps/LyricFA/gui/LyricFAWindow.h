#ifndef LYRICFAWINDOW_H
#define LYRICFAWINDOW_H

#include "AsyncTaskWindow.h"
#include <QCheckBox>
#include <QPushButton>
#include <QSharedPointer>

#include "../util/Asr.h"
#include "../util/MatchLyric.h"
namespace LyricFA {

    class LyricFAWindow : public AsyncTask::AsyncTaskWindow {
        Q_OBJECT
    public:
        explicit LyricFAWindow(QWidget *parent = nullptr);
        ~LyricFAWindow() override;

    protected:
        void init() override;
        void runTask() override;
        void onTaskFinished() override;

    private slots:
        void slot_matchLyric();
        void slot_labPath();
        void slot_jsonPath();
        void slot_lyricPath();

    private:
        QLineEdit *m_labEdit;
        QLineEdit *m_jsonEdit;
        QLineEdit *m_lyricEdit;
        QCheckBox *m_pinyinBox;
        QPushButton *m_matchBtn;

        Asr *m_asr = nullptr;
        QSharedPointer<Pinyin::Pinyin> m_mandarin;
        MatchLyric *m_match = nullptr;

        enum Mode { Mode_Asr, Mode_MatchLyric };
        Mode m_currentMode = Mode_Asr;
    };

} // namespace LyricFA

#endif // LYRICFAWINDOW_H