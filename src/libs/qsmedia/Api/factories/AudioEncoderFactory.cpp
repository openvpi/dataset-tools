#include "AudioEncoderFactory.h"

#include "IAudioEncoder.h"
#include "Api/interfaces/IAudioEncoderPlugin.h"

#include <private/qfactoryloader_p.h>

QSAPI_BEGIN_NAMESPACE

#if QT_CONFIG(settings)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
                          (QsApi_IAudioEncoderPlugin_IID, QLatin1String("/audioencoders"), Qt::CaseInsensitive))
#endif

QStringList AudioEncoderFactory::keys() {
#if QT_CONFIG(settings)
    return loader()->keyMap().values();
#else
    return QStringList();
#endif
}

QString AudioEncoderFactory::requested() {
    QByteArray env = qgetenv("QSAPI_AUDIO_ENCODER");
    return env.isNull() ? "FFmpegEncoder" : QString::fromLocal8Bit(env);
}

IAudioEncoder *AudioEncoderFactory::create(const QString &key, QObject *parent) {
#if QT_CONFIG(settings)
    if (!key.isEmpty()) {
        IAudioEncoder *inst = qLoadPlugin<IAudioEncoder, IAudioEncoderPlugin>(loader(), key, parent);
        if (inst)
            return inst;
        delete inst;
    }
#else
    Q_UNUSED(key);
#endif
    return nullptr;
}

IAudioEncoder *AudioEncoderFactory::create(QObject *parent) {
    return create(requested(), parent);
}

QSAPI_END_NAMESPACE