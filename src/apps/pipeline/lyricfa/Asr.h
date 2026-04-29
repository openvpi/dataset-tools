#ifndef ASR_H
#define ASR_H

#include <memory>
#include <mutex>

#include <QString>
#include <filesystem>

#include <Model.h>

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
#endif // ASR_H