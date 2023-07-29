#ifndef CHORUSKIT_CLEANLYRIC_H
#define CHORUSKIT_CLEANLYRIC_H
#include <QStringList>

namespace IKg2p {
    
    class CleanLyric {
    public:
        static QString reOrder(const QString &rawStr, QList<int> maxChar);
        static QString filterTextInParentheses(const QString &rawStr);
        static QStringList splitLyricAsWord(const QString &rawStr);
        static QStringList splitLyricAsChar(const QString &rawStr);
    };

}

#endif // CHORUSKIT_CLEANLYRIC_H
