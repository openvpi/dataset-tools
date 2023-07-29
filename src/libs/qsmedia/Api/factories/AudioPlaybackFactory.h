#ifndef AUDIOPLAYBACKFACTORYH
#define AUDIOPLAYBACKFACTORYH

#include <QObject>
#include <QStringList>

#include "QsMediaGlobal.h"

QSAPI_BEGIN_NAMESPACE

class IAudioPlayback;

class QSMEDIA_API AudioPlaybackFactory {
public:
    static QStringList keys();
    static QString requested();
    static IAudioPlayback *create(const QString &key, QObject *parent);
    static IAudioPlayback *create(QObject *parent);
};

QSAPI_END_NAMESPACE

#endif // AUDIOPLAYBACKFACTORYH
