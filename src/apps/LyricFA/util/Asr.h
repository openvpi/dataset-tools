#ifndef ASR_H
#define ASR_H

#include <memory>

#include <QString>
#include <filesystem>

#include <Model.h>


namespace LyricFA {

    class Asr {
    public:
        explicit Asr(const QString &modelPath);
        ~Asr();

        bool recognize(const std::filesystem::path &filepath, std::string &msg) const;

    private:
        std::unique_ptr<FunAsr::Model> m_asrHandle;
    };
} // LyricFA

#endif // ASR_H
