#ifndef LYRICDATA_H
#define LYRICDATA_H

#include <QString>
#include <QVector>

namespace LyricFA {

    struct LyricData {
        QVector<QString> text_list;
        QVector<QString> phonetic_list;
        QString raw_text;
    };

} // namespace LyricFA

#endif // LYRICDATA_H