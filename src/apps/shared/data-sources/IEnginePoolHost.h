#pragma once

#include <QString>
#include <atomic>
#include <dstools/ModelManager.h>
#include <memory>
#include <tuple>

class QObject;

namespace dstools {

class EngineAliveToken {
public:
    explicit EngineAliveToken(bool startValid = false) {
        if (startValid)
            m_token = std::make_shared<std::atomic<bool>>(true);
    }

    EngineAliveToken(const EngineAliveToken &) = delete;
    EngineAliveToken &operator=(const EngineAliveToken &) = delete;
    EngineAliveToken(EngineAliveToken &&) noexcept = default;
    EngineAliveToken &operator=(EngineAliveToken &&) noexcept = default;

    [[nodiscard]] std::shared_ptr<std::atomic<bool>> token() const {
        return m_token;
    }

    [[nodiscard]] bool isValid() const {
        return m_token && *m_token;
    }
    explicit operator bool() const {
        return isValid();
    }

    void create() {
        m_token = std::make_shared<std::atomic<bool>>(true);
    }

    void invalidate() {
        if (m_token)
            m_token->store(false);
        m_token.reset();
    }

private:
    std::shared_ptr<std::atomic<bool>> m_token;
};

class IEnginePoolHost {
public:
    static constexpr int kInterfaceVersion = 1;

    virtual ~IEnginePoolHost() = default;

    virtual ModelManager *ensureModelManager() = 0;
    virtual EngineAliveToken &aliveToken(const QString &key) = 0;
    virtual std::tuple<ModelManager *, ModelTypeId> loadModelForTask(const QString &taskKey,
                                                                      const QString &modelTypeName = {}) = 0;
    virtual void onEngineInvalidated(const QString &taskKey) = 0;
    virtual QObject *asQObject() = 0;
};

} // namespace dstools

#define DSTOOLS_ENGINE_POOL_HOST_IID "org.dstools.IEnginePoolHost"
Q_DECLARE_INTERFACE(dstools::IEnginePoolHost, DSTOOLS_ENGINE_POOL_HOST_IID)