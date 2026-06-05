#pragma once
/// @file AudioPlaybackAdapter.h
/// @brief SDL2-based audio playback adapter (non-Qt, no QObject).

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace dsfw::audio {

    /// @brief SDL2-based audio playback adapter.
    ///
    /// Manages SDL2 audio device lifecycle and provides thread-safe PCM data feeding.
    /// Not a QObject — use AudioPlayerAdapter for Qt signal integration.
    class AudioPlaybackAdapter {
    public:
        using StateCallback = std::function<void(bool playing)>;
        using DeviceCallback = std::function<void(const std::string &device)>;

        AudioPlaybackAdapter();
        ~AudioPlaybackAdapter();

        // Non-copyable, movable
        AudioPlaybackAdapter(const AudioPlaybackAdapter &) = delete;
        AudioPlaybackAdapter &operator=(const AudioPlaybackAdapter &) = delete;
        AudioPlaybackAdapter(AudioPlaybackAdapter &&) noexcept;
        AudioPlaybackAdapter &operator=(AudioPlaybackAdapter &&) noexcept;

        /// @brief Initialize the SDL audio subsystem.
        /// @param sampleRate Output sample rate in Hz.
        /// @param channels Number of output channels.
        /// @param bufferSize Audio buffer size in samples.
        /// @return True on success.
        bool setup(int sampleRate = 44100, int channels = 2, int bufferSize = 1024);

        /// @brief Release the SDL audio subsystem.
        void dispose();

        /// @brief Check whether the audio subsystem is initialized.
        [[nodiscard]] bool isSetup() const;

        /// @brief Set audio data for playback (copies data).
        /// @param data Float PCM data (interleaved).
        /// @param frameCount Number of frames.
        /// @param sampleRate Sample rate in Hz.
        /// @param channels Number of channels.
        void setAudioData(const float *data, int64_t frameCount, int sampleRate, int channels);

        /// @brief Clear current audio data.
        void clearAudioData();

        /// @brief Start playback.
        void play();

        /// @brief Stop playback.
        void stop();

        /// @brief Check whether playback is active.
        [[nodiscard]] bool isPlaying() const;

        /// @brief Seek to a position in seconds.
        void setPosition(double sec);

        /// @brief Get the current playback position in seconds.
        [[nodiscard]] double currentTime() const;

        /// @brief Get the total duration of loaded audio in seconds.
        [[nodiscard]] double totalDuration() const;

        /// @brief Get available audio output devices.
        [[nodiscard]] std::vector<std::string> devices() const;

        /// @brief Get the current output device name.
        [[nodiscard]] std::string currentDevice() const;

        /// @brief Set the audio output device.
        void setDevice(const std::string &device);

        /// @brief Set state change callback.
        void setStateCallback(StateCallback cb);

        /// @brief Set device change callback.
        void setDeviceCallback(DeviceCallback cb);

    private:
        struct Impl;
        std::unique_ptr<Impl> d;
    };

} // namespace dsfw::audio