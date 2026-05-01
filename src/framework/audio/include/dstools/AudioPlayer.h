#pragma once
#include <dstools/IAudioPlayer.h>
#include <memory>

namespace dstools::audio {

class AudioDecoder;
class AudioPlayback;

class AudioPlayer : public IAudioPlayer {
    Q_OBJECT
public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer() override;

    bool open(const QString &path) override;
    void close() override;
    bool isOpen() const override;
    double duration() const override;
    double position() const override;
    void setPosition(double sec) override;
    void play() override;
    void stop() override;
    bool isPlaying() const override;
    QStringList devices() const override;
    QString currentDevice() const override;
    void setDevice(const QString &device) override;
    bool setup(int sampleRate, int channels, int bufferSize) override;

private:
    std::unique_ptr<AudioDecoder> m_decoder;
    AudioPlayback *m_playback = nullptr;
    bool m_valid = false;

    void onPlaybackStateChanged();
    void onDeviceChangedInternal();
};

}
