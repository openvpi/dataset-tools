#ifndef AUDIOENCODERFACTORY_H
#define AUDIOENCODERFACTORY_H

#include <QObject>
#include <QStringList>

#include "QsMediaGlobal.h"

QSAPI_BEGIN_NAMESPACE

class IAudioEncoder;

class QSMEDIA_API AudioEncoderFactory {
public:
    static QStringList keys();
    static QString requested();
    static IAudioEncoder *create(const QString &key, QObject *parent);
    static IAudioEncoder *create(QObject *parent);
};

QSAPI_END_NAMESPACE

#endif // AUDIOENCODERFACTORY_H
