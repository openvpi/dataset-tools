#include "AudioDecoderFactory.h"

#include "IAudioDecoder.h"
#include "Api/interfaces/IAudioDecoderPlugin.h"

#include <private/qfactoryloader_p.h>

QSAPI_BEGIN_NAMESPACE

#if QT_CONFIG(settings)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
                          (QsApi_IAudioDecoderPlugin_IID, QLatin1String("/audiodecoders"), Qt::CaseInsensitive))
#endif

QStringList AudioDecoderFactory::keys() {
#if QT_CONFIG(settings)
    return loader()->keyMap().values();
#else
    return QStringList();
#endif
}

QString AudioDecoderFactory::requested() {
    QByteArray env = qgetenv("QSAPI_AUDIO_DECODER");
    return env.isNull() ? "FFmpegDecoder" : QString::fromLocal8Bit(env);
}

IAudioDecoder *AudioDecoderFactory::create(const QString &key, QObject *parent) {
#if QT_CONFIG(settings)
    if (!key.isEmpty()) {
        IAudioDecoder *inst = qLoadPlugin<IAudioDecoder, IAudioDecoderPlugin>(loader(), key, parent);
        if (inst)
            return inst;
        delete inst;
    }
#else
    Q_UNUSED(key);
#endif
    return nullptr;
}

IAudioDecoder *AudioDecoderFactory::create(QObject *parent) {
    return create(requested(), parent);
}

QSAPI_END_NAMESPACE