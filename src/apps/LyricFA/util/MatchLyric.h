#ifndef MATCHLYRIC_H
#define MATCHLYRIC_H

#include <memory>

#include <QPlainTextEdit>
#include <QString>

#include <MandarinG2p.h>

namespace LyricFA {
    class MatchLyric {
    public:
        MatchLyric();
        ~MatchLyric();

        void initLyric(const QString &lyric_folder);

        bool match(const QString& filename, const QString& labPath, const QString& jsonPath, QString &msg,
                   const bool &asr_rectify = true) const;

    private:
        struct lyricInfo {
            QStringList text, pinyin;
        };

        QMap<QString, lyricInfo> m_lyricDict;
        std::unique_ptr<IKg2p::MandarinG2p> m_mandarin;
    };
}
#endif // MATCHLYRIC_H
