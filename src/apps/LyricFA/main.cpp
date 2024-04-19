#include <QCoreApplication>
#include <QDebug>

#include "Asr.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    const QString wavPath = R"(D:\projects\RapidASR\cpp_onnx\test.wav)";
    const QString modelPath =
        R"(D:\projects\RapidASR\cpp_onnx\speech_paraformer-large_asr_nat-zh-cn-16k-common-vocab8404-pytorch)";
    const auto asr = new LyricFA::Asr(modelPath);
    qDebug() << asr->recognize(wavPath);

    return 0;
}
