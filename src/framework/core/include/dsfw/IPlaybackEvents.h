#pragma once

#include <QObject>
#include <functional>

namespace dsfw {

class IPlaybackEvents {
public:
    virtual ~IPlaybackEvents() noexcept = default;

    virtual void registerPlayCallbacks(QObject *context,
                                       std::function<void()> playCb,
                                       std::function<void()> stopCb) = 0;
};

} // namespace dsfw

Q_DECLARE_INTERFACE(dsfw::IPlaybackEvents, "dsfw.IPlaybackEvents/1.0")
