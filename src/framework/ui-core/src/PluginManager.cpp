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
        DSFW_LOG_WARN("PluginManager", ("Plugin directory does not exist: " + dir.toStdString()).c_str());
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
            DSFW_LOG_WARN("PluginManager", ("No metadata in: " + filePath.toStdString()).c_str());
            delete loader;
            continue;
        }

        QObject *instance = loader->instance();
        if (!instance) {
            QString error = loader->errorString();
            DSFW_LOG_WARN("PluginManager", ("Failed to load plugin: " + filePath.toStdString() + " error: " + error.toStdString()).c_str());
            emit pluginLoadFailed(fileName, error);
            delete loader;
            continue;
        }

        auto *plugin = dynamic_cast<IStepPlugin *>(instance);
        if (!plugin) {
            DSFW_LOG_WARN("PluginManager", ("Plugin does not implement IStepPlugin: " + filePath.toStdString()).c_str());
            delete loader;
            continue;
        }

        m_loaders[filePath] = loader;
        m_plugins.append(plugin);
        DSFW_LOG_INFO("PluginManager", ("Loaded plugin: " + filePath.toStdString()).c_str());
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
