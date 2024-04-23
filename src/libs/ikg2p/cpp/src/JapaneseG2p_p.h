#ifndef JPG2PPRIVATE_H
#define JPG2PPRIVATE_H

#include <QHash>

#include "JapaneseG2p_p.h"

namespace IKg2p {

    class JpG2pPrivate {
        Q_DECLARE_PUBLIC(JapaneseG2p)
    public:
        JpG2pPrivate();
        virtual ~JpG2pPrivate();

        void init();

        JapaneseG2p *q_ptr;

        QHash<QString, QString> kanaToRomajiMap;
        QHash<QString, QString> romajiToKanaMap;

        enum KanaType { Hiragana, Katakana };

        static QStringList convertKana(const QStringList &kanaList, KanaType kanaType);
    };
}

#endif // JPG2PPRIVATE_H