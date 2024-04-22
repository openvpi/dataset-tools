#ifndef ASR_H
#define ASR_H

#include <QString>

#include <Model.h>

namespace LyricFA {
    struct QVIO {
        qint64 seek{};
        QByteArray byteArray;
    };

    class Asr {
    public:
        explicit Asr(const QString &modelPath);
        ~Asr();

        [[nodiscard]] bool recognize(const QString &filename, QString &msg) const;
        [[nodiscard]] bool recognize(const QVIO &qvio, QString &msg) const;

    private:
        [[nodiscard]] static QVIO resample(const QString &filename);

        Model* m_asrHandle;
    };
} // LyricFA

#endif // ASR_H
