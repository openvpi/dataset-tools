#ifndef ASR_H
#define ASR_H

#include <memory>

#include <QString>

#include <Model.h>

#include "SndfileVio.h"

namespace LyricFA {

    class Asr {
    public:
        explicit Asr(const QString &modelPath);
        ~Asr();

        [[nodiscard]] bool recognize(const QString &filename, QString &msg) const;
        [[nodiscard]] bool recognize(SF_VIO sf_vio, QString &msg) const;

    private:
        [[nodiscard]] static SF_VIO resample(const QString &filename);

        std::unique_ptr<FunAsr::Model> m_asrHandle;
    };
} // LyricFA

#endif // ASR_H
