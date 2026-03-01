#ifndef MATCHLYRIC_H
#define MATCHLYRIC_H

#include "LyricMatcher.h"
#include <QMap>
#include <QString>
#include <memory>

namespace LyricFA {
    class MatchLyric {
    public:
        MatchLyric();
        ~MatchLyric();

        void initLyric(const QString &lyric_folder);
        bool match(const QString &filename, const QString &labPath, const QString &jsonPath, QString &msg) const;

    private:
        struct LyricInfo {
            QVector<QString> text;
            QVector<QString> pinyin;
        };

        QMap<QString, LyricInfo> m_lyricDict;
        std::unique_ptr<LyricMatcher> m_matcher;
        int m_diffThreshold = 1;
    };
}
#endif // MATCHLYRIC_H