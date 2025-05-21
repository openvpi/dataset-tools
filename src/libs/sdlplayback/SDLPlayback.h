#ifndef SDLPLAYBACK_H
#define SDLPLAYBACK_H

#include "Api/IAudioPlayback.h"

QSAPI_USING_NAMESPACE

class SDLPlaybackPrivate;

class SDLPlayback final : public IAudioPlayback {
    Q_OBJECT
    Q_DECLARE_PRIVATE(SDLPlayback)
public:
    explicit SDLPlayback(QObject *parent = nullptr);
    ~SDLPlayback() override;

public:
    QStringList drivers() const override;
    QString currentDriver() const override;
    bool setDriver(const QString &driver) override;

    QStringList devices() const override;
    QString currentDevice() const override;
    bool setDevice(const QString &device) override;

protected:
    explicit SDLPlayback(SDLPlaybackPrivate &d, QObject *parent = nullptr);
};

#endif // SDLPLAYBACK_H
