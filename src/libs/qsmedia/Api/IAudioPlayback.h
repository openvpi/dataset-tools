#ifndef IAUDIOPLAYBACK_H
#define IAUDIOPLAYBACK_H

#include <QObject>

#include "Api/IAudioDecoder.h"
#include "QsMediaNamespace.h"

QSAPI_BEGIN_NAMESPACE

class IAudioPlaybackPrivate;

class QSMEDIA_API IAudioPlayback : public QObject {
    Q_OBJECT
    Q_DECLARE_PRIVATE(IAudioPlayback)
public:
    explicit IAudioPlayback(QObject *parent = nullptr);
    ~IAudioPlayback();

    enum PlaybackState {
        Playing,
        Stopped,
    };

public:
    /**
     * @brief Open audio device and get ready
     *
     * @param args Audio device arguments
     */
    bool setup(const QsMedia::PlaybackArguments &args);

    /**
     * @brief Close audio device
     */
    void dispose();

    /**
     * @brief Return if the audio device is ready
     */
    bool isAvailable() const;

    void setDecoder(IAudioDecoder *decoder);

    /**
     * @brief Return if the decoder is ready
     */
    bool isReady() const;

    void play();
    void stop();

    PlaybackState state() const;
    bool isPlaying() const;

    virtual QStringList drivers() const;
    virtual QString currentDriver() const;
    virtual bool setDriver(const QString &driver);

    virtual QStringList devices() const;
    virtual QString currentDevice() const;
    virtual bool setDevice(const QString &device);

    int bufferSamples() const;
    int sampleRate() const;
    int channels() const;

protected:
    IAudioPlayback(IAudioPlaybackPrivate &d, QObject *parent = nullptr);

    QScopedPointer<IAudioPlaybackPrivate> d_ptr;

signals:
    void stateChanged();
    void driverChanged();

    void deviceChanged();
    void deviceAdded();
    void deviceRemoved();
};

QSAPI_END_NAMESPACE

#define QsApi_IAudioPlayback_IID "org.ChorusKit.QsLib.IAudioPlayback"

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(QsApi::IAudioPlayback, QsApi_IAudioPlayback_IID)
QT_END_NAMESPACE

#endif // IAUDIOPLAYBACK_H
