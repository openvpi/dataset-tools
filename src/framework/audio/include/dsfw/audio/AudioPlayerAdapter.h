#pragma once
/// @file AudioPlayerAdapter.h
/// @brief Concrete audio player combining FfmpegAudioDecoder + AudioPlaybackAdapter.

#include <dsfw/audio/IAudioPlayerAdapter.h>
#include <memory>

namespace dsfw::audio {

    class AudioPlaybackAdapter;

    /// @brief Concrete audio player adapter.
    ///
    /// Combines FfmpegAudioDecoder + SwresampleResampler + AudioPlaybackAdapter.
    /// Decodes entire file on open() and feeds resampled PCM data to the playback engine.
    class AudioPlayerAdapter : public IAudioPlayerAdapter {
        Q_OBJECT
    public:
        explicit AudioPlayerAdapter(QObject *parent = nullptr);
        ~AudioPlayerAdapter() override;

        // IAudioPlayerAdapter impl
        dsfw::Result<void> open(const std::filesystem::path &path) override;
        void close() override;
        [[nodiscard]] bool isOpen() const override;
        [[nodiscard]] double duration() const override;
        [[nodiscard]] double position() const override;
        void setPosition(double sec) override;
        void play() override;
        void stop() override;
        [[nodiscard]] bool isPlaying() const override;
        [[nodiscard]] QStringList devices() const override;
        [[nodiscard]] QString currentDevice() const override;
        void setDevice(const QString &device) override;
        bool setup(int sampleRate, int channels, int bufferSize) override;

    private:
        struct Impl;
        std::unique_ptr<Impl> d;
    };

} // namespace dsfw::audio