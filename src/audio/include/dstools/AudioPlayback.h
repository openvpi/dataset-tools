#pragma once
#include <QObject>
#include <QStringList>
#include <memory>

namespace dstools::audio {

class AudioDecoder;

/// Audio player. Uses SDL2 internally.
/// Replaces original qsmedia::IAudioPlayback + SDLPlayback plugin.
///
/// Bug fixes vs original:
///   - std::atomic<int> replaces volatile int state (BUG-018)
///   - std::condition_variable replaces busy-wait (BUG-018)
///   - std::lock_guard replaces manual lock/unlock (BUG-019)
///   - Empty device list guard (BUG-020)
///   - pcm_buffer allocation fix (CQ-011)
class AudioPlayback : public QObject {
    Q_OBJECT
public:
    explicit AudioPlayback(QObject *parent = nullptr);
    ~AudioPlayback() override;

    AudioPlayback(const AudioPlayback &) = delete;
    AudioPlayback &operator=(const AudioPlayback &) = delete;

    /// Initialize audio subsystem.
    /// @return false means initialization failed (all playback methods become no-ops).
    bool setup(int sampleRate = 44100, int channels = 2, int bufferSize = 1024);

    /// Release audio subsystem.
    void dispose();

    /// Set decoder source
    void setDecoder(AudioDecoder *decoder);

    /// Playback control
    void play();
    void stop();

    /// Device management
    QStringList devices() const;
    QString currentDevice() const;
    void setDevice(const QString &device);

    /// State
    enum State { Stopped, Playing, Paused };
    State state() const;

signals:
    void stateChanged(AudioPlayback::State newState);
    void deviceChanged(const QString &device);
    void deviceAdded(const QString &device);
    void deviceRemoved(const QString &device);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace dstools::audio
