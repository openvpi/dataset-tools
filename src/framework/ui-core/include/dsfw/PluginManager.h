#pragma once

#include <QObject>
#include <QHash>
#include <QList>
#include <QString>

class QPluginLoader;

namespace dsfw {

class IStepPlugin;

class PluginManager : public QObject {
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);
    ~PluginManager() override;

    void loadFrom(const QString &dir);
    void unloadAll();

    QList<IStepPlugin *> plugins() const;

    template <typename T>
    QList<T *> plugins() const;

    QStringList loadedFileNames() const;

signals:
    void pluginLoaded(const QString &fileName, IStepPlugin *plugin);
    void pluginLoadFailed(const QString &fileName, const QString &error);

private:
    QHash<QString, QPluginLoader *> m_loaders;
    QList<IStepPlugin *> m_plugins;
};

template <typename T>
QList<T *> PluginManager::plugins() const {
    QList<T *> result;
    for (auto *p : m_plugins) {
        if (auto *casted = dynamic_cast<T *>(p))
            result.append(casted);
    }
    return result;
}

} // namespace dsfw
