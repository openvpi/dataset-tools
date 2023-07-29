#include "SDLPlayback.h"
#include "private/SDLPlayback_p.h"

#include <QDebug>

SDLPlayback::SDLPlayback(QObject *parent) : SDLPlayback(*new SDLPlaybackPrivate(), parent) {
}

SDLPlayback::~SDLPlayback() {
    Q_D(SDLPlayback);
    if (d->available) {
        dispose();
    }
}

QStringList SDLPlayback::drivers() const {
    QStringList res;
    int cnt = SDL_GetNumAudioDrivers();
    for (int i = 0; i < cnt; i++) {
        QString drv = QString::fromStdString(SDL_GetAudioDriver(i));
        if (drv == "dummy" || drv == "disk") {
            continue;
        }
        res.append(drv);
    }
    return res;
}

QString SDLPlayback::currentDriver() const {
    Q_D(const SDLPlayback);
    return d->driver;
}

bool SDLPlayback::setDriver(const QString &driver) {
    Q_D(SDLPlayback);

    //    if (state() == Playing) {
    //        qDebug().noquote() << "SDLPlayback: Don't change audio driver when playing.";
    //        return false;
    //    }

    stop();

    return d->switchDriver(driver);
}

QStringList SDLPlayback::devices() const {
    QStringList res;
    int cnt = SDL_GetNumAudioDevices(0);
    for (int i = 0; i < cnt; i++) {
        res.append(QString::fromStdString(SDL_GetAudioDeviceName(i, 0)));
    }
    return res;
}

QString SDLPlayback::currentDevice() const {
    Q_D(const SDLPlayback);
    return d->device;
}

bool SDLPlayback::setDevice(const QString &device) {
    Q_D(SDLPlayback);

    //    if (state() == Playing) {
    //        qDebug().noquote() << "SDLPlayback: Don't change audio device when playing.";
    //        return false;
    //    }

    stop();

    return d->switchDevId(device);
}

SDLPlayback::SDLPlayback(SDLPlaybackPrivate &d, QObject *parent) : IAudioPlayback(d, parent) {
    d.init();
}
