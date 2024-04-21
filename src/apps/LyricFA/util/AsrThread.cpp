#include "AsrThread.h"

#include <QApplication>
#include <QMSystem.h>
#include <QMessageBox>

AsrThread::AsrThread(LyricFA::Asr *asr, QString filename, QString wavPath, QString labPath)
    : m_asr(asr), m_filename(std::move(filename)), m_wavPath(std::move(wavPath)), m_labPath(std::move(labPath)) {
}

void AsrThread::run() {
    QString asrMsg;
    const auto asrRes = m_asr->recognize(m_wavPath, asrMsg);

    if (!asrRes) {
        Q_EMIT this->oneFailed(m_filename, asrMsg);
        return;
    }

    QFile labFile(m_labPath);
    if (!labFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, QApplication::applicationName(), QString("Failed to write lab file."));
        return;
    }

    QTextStream labIn(&labFile);
    labIn.setCodec(QTextCodec::codecForName("UTF-8"));
    labIn << asrRes;
    labFile.close();
    Q_EMIT this->oneFinished(m_filename, asrMsg);
}
