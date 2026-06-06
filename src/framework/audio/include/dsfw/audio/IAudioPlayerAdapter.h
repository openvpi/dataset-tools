#pragma once
/// @file IAudioPlayerAdapter.h
/// @brief Abstract audio player interface with Qt signals.

#include <QObject>
#include <QString>
#include <QStringList>
#include <dstools/Result.h>
#include <filesystem>

namespace dsfw::audio {

    /// @brief Abstract audio player interface.
    ///
    /// PlayWidget depends on this interface, allowing different implementations
    /// (owned or external player) via constructor injection (INFRA-07).
    class IAudioPlayerAdapter : public QObject {
        Q_OBJECT
    public:
        /// @brief Interface version for forward compatibility (INFRA-14).
        static constexpr int kInterfaceVersion = 1;

        /// @brief Playback states.
        enum class State { Stopped, Playing };

        explicit IAudioPlayerAdapter(QObject *parent = nullptr) : QObject(parent) {}
        ~IAudioPlayerAdapter() override = default;

        /// @brief Open an audio file.
        /// @param path Path to the audio file.
        /// @return Result indicating success or error.
        virtual dsfw::Result<void> open(const std::filesystem::path &path) = 0;

        /// @brief Close the current audio file.
        virtual void close() = 0;

        /// @brief Check whether a file is open.
        [[nodiscard]] virtual bool isOpen() const = 0;

        /// @brief Get total duration in seconds.
        [[nodiscard]] virtual double duration() const = 0;

        /// @brief Get the current playback position in seconds.
        [[nodiscard]] virtual double position() const = 0;

        /// @brief Seek to a position in seconds.
        virtual void setPosition(double sec) = 0;

        /// @brief Start playback.
        virtual void play() = 0;

        /// @brief Stop playback.
        virtual void stop() = 0;

        /// @brief Check whether playback is active.
        [[nodiscard]] virtual bool isPlaying() const = 0;

        /// @brief Get available audio output devices.
        [[nodiscard]] virtual QStringList devices() const = 0;

        /// @brief Get the current output device name.
        [[nodiscard]] virtual QString currentDevice() const = 0;

        /// @brief Set the audio output device.
        virtual void setDevice(const QString &device) = 0;

        /// @brief Initialize the audio subsystem.
        /// @return True on success.
        virtual bool setup(int sampleRate, int channels, int bufferSize) = 0;

    signals:
        /// @brief Emitted when the playback state changes.
        void stateChanged(IAudioPlayerAdapter::State newState);

        /// @brief Emitted when the output device changes.
        void deviceChanged(const QString &device);
    };

} // namespace dsfw::audio