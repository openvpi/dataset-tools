#ifndef MATCHLYRIC_H
#define MATCHLYRIC_H

#include <memory>

#include <QPlainTextEdit>
#include <QString>

#include <cpp-pinyin/Pinyin.h>

#include "../util/LyricAligner.h"


namespace LyricFA {
    class MatchLyric {
    public:
        MatchLyric();
        ~MatchLyric();

        void initLyric(const QString &lyric_folder);

        bool match(const QString &filename, const QString &labPath, const QString &jsonPath, QString &msg,
                   const bool &asr_rectify = true) const;

    private:
        struct lyricInfo {
            std::vector<std::string> text, pinyin;
        };

        LyricAligner *m_lyricAligner;
        QMap<QString, lyricInfo> m_lyricDict;
        std::unique_ptr<Pinyin::Pinyin> m_mandarin;
    };
}
#endif // MATCHLYRIC_H
