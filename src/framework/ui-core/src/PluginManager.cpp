#include <dsfw/PluginManager.h>
#include <dsfw/IStepPlugin.h>
#include <dsfw/Log.h>

#include <QDir>
#include <QPluginLoader>
#include <QJsonObject>

namespace dsfw {

PluginManager::PluginManager(QObject *parent) : QObject(parent) {}

PluginManager::~PluginManager() {
    unloadAll();
}

void PluginManager::loadFrom(const QString &dir) {
    QDir pluginDir(dir);
    if (!pluginDir.exists()) {
        Log::warn("PluginManager") << "Plugin directory does not exist: " << dir.toStdString();
        return;
    }

    const auto entries = pluginDir.entryList(QDir::Files);
    for (const QString &fileName : entries) {
        QString filePath = pluginDir.absoluteFilePath(fileName);

        if (m_loaders.contains(filePath))
            continue;

        auto *loader = new QPluginLoader(filePath, this);
        QJsonObject meta = loader->metaData();
        if (meta.isEmpty()) {
            Log::warn("PluginManager") << "No metadata in: " << filePath.toStdString();
            delete loader;
            continue;
        }

        QObject *instance = loader->instance();
        if (!instance) {
            QString error = loader->errorString();
            Log::warn("PluginManager") << "Failed to load plugin: " << filePath.toStdString()
                                       << " error: " << error.toStdString();
            emit pluginLoadFailed(fileName, error);
            delete loader;
            continue;
        }

        auto *plugin = qobject_cast<IStepPlugin *>(instance);
        if (!plugin) {
            Log::warn("PluginManager") << "Plugin does not implement IStepPlugin: "
                                       << filePath.toStdString();
            delete loader;
            continue;
        }

        m_loaders[filePath] = loader;
        m_plugins.append(plugin);
        Log::info("PluginManager") << "Loaded plugin: " << filePath.toStdString();
        emit pluginLoaded(fileName, plugin);
    }
}

void PluginManager::unloadAll() {
    for (auto it = m_loaders.begin(); it != m_loaders.end(); ++it) {
        it.value()->unload();
        delete it.value();
    }
    m_loaders.clear();
    m_plugins.clear();
}

QList<IStepPlugin *> PluginManager::plugins() const {
    return m_plugins;
}

QStringList PluginManager::loadedFileNames() const {
    return m_loaders.keys();
}

} // namespace dsfw
