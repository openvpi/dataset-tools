#include "AudioPlaybackFactory.h"

#include "IAudioPlayback.h"
#include "Api/interfaces/IAudioPlaybackPlugin.h"

#include <private/qfactoryloader_p.h>

QSAPI_BEGIN_NAMESPACE

#if QT_CONFIG(settings)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
                          (QsApi_IAudioPlaybackPlugin_IID, QLatin1String("/audioplaybacks"), Qt::CaseInsensitive))
#endif

QStringList AudioPlaybackFactory::keys() {
#if QT_CONFIG(settings)
    return loader()->keyMap().values();
#else
    return QStringList();
#endif
}

QString AudioPlaybackFactory::requested() {
    QByteArray env = qgetenv("QSAPI_AUDIO_PLAYBACK");
    return env.isNull() ? "SDLPlayback" : QString::fromLocal8Bit(env);
}

IAudioPlayback *AudioPlaybackFactory::create(const QString &key, QObject *parent) {
#if QT_CONFIG(settings)
    if (!key.isEmpty()) {
        IAudioPlayback *inst = qLoadPlugin<IAudioPlayback, IAudioPlaybackPlugin>(loader(), key, parent);
        if (inst)
            return inst;
        delete inst;
    }
#else
    Q_UNUSED(key);
#endif
    return nullptr;
}

IAudioPlayback *AudioPlaybackFactory::create(QObject *parent) {
    return create(requested(), parent);
}

QSAPI_END_NAMESPACE