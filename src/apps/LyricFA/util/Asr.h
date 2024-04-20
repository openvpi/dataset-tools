#ifndef ASR_H
#define ASR_H

#include <QString>

#include <librapidasrapi.h>

namespace LyricFA {
    struct QVIO {
        qint64 seek{};
        QByteArray byteArray;
    };

    class Asr {
    public:
        explicit Asr(const QString &modelPath);
        ~Asr();

        [[nodiscard]] QString recognize(const QString &filename) const;
        [[nodiscard]] QString recognize(const QVIO &qvio) const;

    private:
        [[nodiscard]] static QVIO resample(const QString &filename);

        RPASR_HANDLE m_asrHandle;
    };
} // LyricFA

#endif // ASR_H
