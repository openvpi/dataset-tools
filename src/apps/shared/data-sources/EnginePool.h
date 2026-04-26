#pragma once

#include "AppSettingsBackend.h"
#include "IEnginePoolHost.h"
#include "ModelConfigHelper.h"
#include <InferBridge.h>
#include <QObject>
#include <QString>
#include <any>
#include <atomic>
#include <dstools/ModelManager.h>
#include <dsfw/IModelProvider.h>
#include <map>
#include <memory>

namespace dstools {

class EnginePool {
public:
    explicit EnginePool(IEnginePoolHost *page) : m_page(page) {
    }

    EnginePool(ModelManager *mgr, AppSettingsBackend *settings, QObject *owner = nullptr)
        : m_mgr(mgr), m_settings(settings), m_owner(owner) {
    }

    template <typename EngineType>
    EngineType *acquire(const QString &modelKey) {
        if (m_page)
            return acquireViaPage<EngineType>(modelKey);
        return acquireStandalone<EngineType>(modelKey);
    }

    template <typename EngineType>
    bool isOpen(const QString &modelKey) const {
        using Traits = EngineTraits<EngineType>;

        auto it = m_entries.find(modelKey);
        if (it == m_entries.end())
            return false;

        auto *engine = std::any_cast<EngineType *>(it->second.engine);
        return Traits::isOpen(engine);
    }

    void invalidate(const QString &modelKey) {
        m_entries.erase(modelKey);
    }

    void shutdown() {
        for (auto &[key, entry] : m_entries) {
            if (entry.aliveToken)
                entry.aliveToken->store(false);
        }
        m_entries.clear();
        m_standaloneTokens.clear();
    }

    bool engineIsOpen(const QString &modelKey) const {
        return m_entries.find(modelKey) != m_entries.end();
    }

    std::shared_ptr<std::atomic<bool>> aliveToken(const QString &modelKey) {
        return m_standaloneTokens[modelKey];
    }

private:
    struct PoolEntry {
        std::any engine;
        std::shared_ptr<std::atomic<bool>> aliveToken;
    };

    template <typename EngineType>
    EngineType *acquireViaPage(const QString &modelKey) {
        using Traits = EngineTraits<EngineType>;

        auto it = m_entries.find(modelKey);
        if (it != m_entries.end()) {
            auto *engine = std::any_cast<EngineType *>(it->second.engine);
            if (Traits::isOpen(engine))
                return engine;
        }

        auto *mgr = m_page->ensureModelManager();
        if (!mgr)
            return nullptr;

        auto &token = m_page->aliveToken(modelKey);
        if (!token.isValid()) {
            QObject::connect(mgr, &ModelManager::modelInvalidated, m_page->asQObject(),
                             [this](const QString &taskKey) {
                                 m_page->onEngineInvalidated(taskKey);
                             });
        }

        auto [mm, typeId] = m_page->loadModelForTask(modelKey);
        if (!mm || !typeId.isValid())
            return nullptr;

        auto *provider = mm->provider(typeId);
        auto *typedProvider = dynamic_cast<typename Traits::ProviderType *>(provider);
        if (typedProvider && Traits::isOpen(&typedProvider->engine())) {
            auto *engine = &typedProvider->engine();
            token.create();
            m_entries[modelKey] = {engine, token.token()};
            return engine;
        }

        return nullptr;
    }

    template <typename EngineType>
    EngineType *acquireStandalone(const QString &modelKey) {
        using Traits = EngineTraits<EngineType>;

        auto it = m_entries.find(modelKey);
        if (it != m_entries.end()) {
            auto *engine = std::any_cast<EngineType *>(it->second.engine);
            if (Traits::isOpen(engine))
                return engine;
        }

        if (!m_mgr)
            return nullptr;

        auto config = readModelConfig(m_settings, modelKey);
        if (config.modelPath.isEmpty())
            return nullptr;

        auto &token = m_standaloneTokens[modelKey];
        if (!token) {
            token = std::make_shared<std::atomic<bool>>(true);
            if (m_owner) {
                QObject::connect(m_mgr, &ModelManager::modelInvalidated, m_owner, [this](const QString &) {
                    shutdown();
                });
            }
        }

        auto result = m_mgr->loadModel(modelKey, config, config.deviceId);
        if (!result)
            return nullptr;

        auto typeId = registerModelType(modelKey.toStdString());

        auto *provider = m_mgr->provider(typeId);
        auto *typedProvider = dynamic_cast<typename Traits::ProviderType *>(provider);
        if (typedProvider && Traits::isOpen(&typedProvider->engine())) {
            auto *engine = &typedProvider->engine();
            m_entries[modelKey] = {engine, token};
            return engine;
        }

        return nullptr;
    }

    IEnginePoolHost *m_page = nullptr;
    ModelManager *m_mgr = nullptr;
    AppSettingsBackend *m_settings = nullptr;
    QObject *m_owner = nullptr;
    std::map<QString, PoolEntry> m_entries;
    std::map<QString, std::shared_ptr<std::atomic<bool>>> m_standaloneTokens;
};

} // namespace dstools