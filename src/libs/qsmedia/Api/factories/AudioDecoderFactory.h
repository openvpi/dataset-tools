#ifndef AUDIODECODERFACTORY_H
#define AUDIODECODERFACTORY_H

#include <QObject>
#include <QStringList>

#include "QsMediaGlobal.h"

QSAPI_BEGIN_NAMESPACE

class IAudioDecoder;

class QSMEDIA_API AudioDecoderFactory {
public:
    static QStringList keys();
    static QString requested();
    static IAudioDecoder *create(const QString &key, QObject *parent);
    static IAudioDecoder *create(QObject *parent);
};

QSAPI_END_NAMESPACE

#endif // AUDIODECODERFACTORY_H
