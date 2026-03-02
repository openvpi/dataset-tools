#include "AudioEncoderFactory.h"
#include "Api/interfaces/IAudioEncoderPlugin.h"
#include "IAudioEncoder.h"

#include <QCoreApplication>
#include <QDir>
#include <QGlobalStatic>
#include <QJsonArray>
#include <QList>
#include <QPluginLoader>

QSAPI_BEGIN_NAMESPACE

struct PluginInfo {
    QPluginLoader *loader;
    IAudioEncoderPlugin *instance;
    QStringList keys;
};

Q_GLOBAL_STATIC(QList<PluginInfo>, pluginList)

static void ensurePluginsLoaded() {
    if (!pluginList()->isEmpty())
        return;

    const QStringList paths = QCoreApplication::libraryPaths();
    for (const QString &path : paths) {
        QDir pluginDir(path + QLatin1String("/audioencoders"));
        if (!pluginDir.exists())
            continue;

        const QStringList entries = pluginDir.entryList(QDir::Files);
        for (const QString &fileName : entries) {
            if (!QLibrary::isLibrary(fileName))
                continue;

            QString filePath = pluginDir.absoluteFilePath(fileName);
            auto *loader = new QPluginLoader(filePath);
            if (!loader->load()) {
                delete loader;
                continue;
            }

            QObject *root = loader->instance();
            if (!root) {
                loader->unload();
                delete loader;
                continue;
            }

            auto *plugin = qobject_cast<IAudioEncoderPlugin *>(root);
            if (!plugin) {
                loader->unload();
                delete loader;
                continue;
            }

            QJsonObject meta = loader->metaData().value("MetaData").toObject();
            QJsonValue keysVal = meta.value("Keys");
            QStringList keys;
            if (keysVal.isArray()) {
                for (const QJsonValue &v : keysVal.toArray())
                    keys << v.toString();
            } else if (keysVal.isString()) {
                keys << keysVal.toString();
            }

            if (keys.isEmpty()) {
                loader->unload();
                delete loader;
                continue;
            }

            PluginInfo info;
            info.loader = loader;
            info.instance = plugin;
            info.keys = keys;
            pluginList()->append(info);
        }
    }
}

QStringList AudioEncoderFactory::keys() {
    ensurePluginsLoaded();
    QStringList result;
    for (const PluginInfo &info : *pluginList())
        result.append(info.keys);
    return result;
}

QString AudioEncoderFactory::requested() {
    const QByteArray env = qgetenv("QSAPI_AUDIO_ENCODER");
    return env.isNull() ? "FFmpegEncoder" : QString::fromLocal8Bit(env);
}

IAudioEncoder *AudioEncoderFactory::create(const QString &key, QObject *parent) {
    ensurePluginsLoaded();
    for (const PluginInfo &info : *pluginList()) {
        if (std::any_of(info.keys.begin(), info.keys.end(),
                        [&key](const QString &k) { return k.compare(key, Qt::CaseInsensitive) == 0; })) {
            return info.instance->create(key, parent);
        }
    }
    return nullptr;
}

IAudioEncoder *AudioEncoderFactory::create(QObject *parent) {
    return create(requested(), parent);
}

QSAPI_END_NAMESPACE