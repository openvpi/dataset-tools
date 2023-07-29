#include "QMCodec.h"

namespace QMCodec {

    QTextCodec *GetUtfCodec(const QByteArray &data, bool *determined) {
        QTextCodec *autoCodec = QTextCodec::codecForUtfText(data);
        QString name = autoCodec->name();
        if (name == "ISO-8859-1") {
            autoCodec = nullptr;
        } else {
            determined ? (*determined = true) : 0;
            return autoCodec;
        }

        QList<QTextCodec *> codecs{QTextCodec::codecForName("UTF-8")};
        for (auto it = codecs.begin(); it != codecs.end(); ++it) {
            QTextCodec::ConverterState state;
            QTextCodec *codec = *it;
            QString text = codec->toUnicode(data.constData(), data.size(), &state);
            Q_UNUSED(text)
            if (state.invalidChars == 0) {
                autoCodec = codec;
                break;
            }
        }

        determined ? (*determined = false) : 0; // Not certain
        return autoCodec;
    }

    QString unescape(const QString &s) {
        QString res;
        auto p = s.data();
        while (!p->isNull()) {
            if (*p == '\\') {
                auto pNext = p + 1;
                if (!pNext->isNull()) {
                    res += *pNext;
                    p += 2;
                } else {
                    break;
                }
            } else {
                res += *p;
                p++;
            }
        }
        return res;
    }

    QByteArray toNativePath(const QByteArray &s) {
        QByteArray s2 = s;
#ifdef Q_OS_WINDOWS
        return s2.replace('/', '\\');
#else
        return fromNativePath(s);
#endif
    }

    QByteArray fromNativePath(const QByteArray &s) {
        QByteArray s2 = s;
        return s2.replace('\\', '/');
    }

}