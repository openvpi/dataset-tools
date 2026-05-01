#ifndef ASR_H
#define ASR_H

/// @file Asr.h
/// @brief FunASR speech recognition engine wrapper.

#include <memory>
#include <mutex>

#include <QString>
#include <filesystem>

#include <Model.h>

namespace LyricFA {

    /// @brief Wraps FunASR Model for audio-to-text recognition, thread-safe.
    class Asr {
    public:
        /// @brief Construct an ASR engine and load the model.
        /// @param modelPath ASR model directory.
        /// @param provider Execution provider (CPU by default).
        /// @param deviceId GPU device index.
        explicit Asr(const QString &modelPath,
                     FunAsr::ExecutionProvider provider = FunAsr::ExecutionProvider::CPU,
                     int deviceId = 0);
        ~Asr();

        /// @brief Recognize speech from an audio file.
        /// @param filepath Path to the audio file.
        /// @param msg Recognition result or error message.
        /// @return True on success.
        bool recognize(const std::filesystem::path &filepath, std::string &msg) const;

        /// @brief Check if the ASR model is loaded.
        /// @return True if the model is initialized.
        bool initialized() const {
            return m_asrHandle != nullptr;
        }

    private:
        std::unique_ptr<FunAsr::Model> m_asrHandle; ///< Underlying FunASR model handle.
        mutable std::mutex m_mutex;                  ///< Mutex for thread-safe recognition.
    };
} // LyricFA
#endif // ASR_H