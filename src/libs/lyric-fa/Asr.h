#pragma once
/// @file Asr.h
/// @brief FunASR speech recognition engine wrapper.

#include <memory>
#include <mutex>

#include <QString>
#include <filesystem>

#include <dstools/ExecutionProvider.h>

namespace FunAsr {
    class Model;
    using ExecutionProvider = dstools::infer::ExecutionProvider;
}

namespace LyricFA {

    class Asr {
    public:
        explicit Asr(const QString &modelPath,
                     FunAsr::ExecutionProvider provider = FunAsr::ExecutionProvider::CPU,
                     int deviceId = 0);
        ~Asr();

        bool recognize(const std::filesystem::path &filepath, std::string &msg) const;

        bool initialized() const {
            return m_asrHandle != nullptr;
        }

    private:
        std::unique_ptr<FunAsr::Model> m_asrHandle;
        mutable std::mutex m_mutex;
    };
} // LyricFA
