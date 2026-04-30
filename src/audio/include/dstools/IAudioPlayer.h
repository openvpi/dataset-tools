#pragma once
#include <QObject>
#include <QStringList>
#include <memory>

namespace dstools::audio {

class IAudioPlayer : public QObject {
    Q_OBJECT
public:
    explicit IAudioPlayer(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IAudioPlayer() = default;

    virtual bool open(const QString &path) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual double duration() const = 0;
    virtual double position() const = 0;
    virtual void setPosition(double sec) = 0;
    virtual void play() = 0;
    virtual void stop() = 0;
    virtual bool isPlaying() const = 0;
    virtual QStringList devices() const = 0;
    virtual QString currentDevice() const = 0;
    virtual void setDevice(const QString &device) = 0;
    virtual bool setup(int sampleRate, int channels, int bufferSize) = 0;

signals:
    void stateChanged();
    void deviceChanged(const QString &device);
};

}
