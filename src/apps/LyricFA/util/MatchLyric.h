#ifndef MATCHLYRIC_H
#define MATCHLYRIC_H

#include <QPlainTextEdit>
#include <QString>

#include <mandarin.h>

class MatchLyric {
public:
    MatchLyric();
    ~MatchLyric();

    void match(QPlainTextEdit *out, const QString &lyric_folder, const QString &lab_folder, const QString &json_folder,
               const bool &asr_rectify = true) const;

private:
    IKg2p::Mandarin *m_mandarin;
};

#endif // MATCHLYRIC_H
