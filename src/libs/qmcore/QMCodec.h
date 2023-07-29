#ifndef QMCODEC_H
#define QMCODEC_H

#include <QByteArray>
#include <QTextCodec>

#include "QMGlobal.h"

namespace QMCodec {

    /**
     * @brief Get the Utf Codec of data (Deprecated)
     *
     * @param data bytes
     * @param determined If the target codec is determined
     * @return
     */
    QMCORE_EXPORT QTextCodec *GetUtfCodec(const QByteArray &data, bool *determined = nullptr);

    /**
     * @brief Convert escaped string in C++ to literal
     *
     * @param s string
     */
    QMCORE_EXPORT QString unescape(const QString &s);

    QMCORE_EXPORT QByteArray toNativePath(const QByteArray &s);

    QMCORE_EXPORT QByteArray fromNativePath(const QByteArray &s);

}

#endif // QMCODEC_H
