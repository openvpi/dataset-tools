#include "AsrThread.h"

#include <QApplication>
#include <QMSystem.h>
#include <QMessageBox>

namespace LyricFA {
    AsrThread::AsrThread(Asr *asr, QString filename, QString wavPath, QString labPath,
                         const QSharedPointer<Pinyin::Pinyin> &g2p)
        : m_asr(asr), m_filename(std::move(filename)), m_wavPath(std::move(wavPath)), m_labPath(std::move(labPath)),
          m_g2p(g2p) {
    }

    void AsrThread::run() {
        std::string asrMsg;
        const auto asrRes = m_asr->recognize(m_wavPath.toLocal8Bit().toStdString(), asrMsg);

        if (!asrRes) {
            Q_EMIT this->oneFailed(m_filename, QString::fromStdString(asrMsg));
            return;
        }

        QFile labFile(m_labPath.toLocal8Bit());
        if (!labFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(nullptr, QApplication::applicationName(),
                                  QString("Failed to write to file %1").arg(QMFs::PathFindFileName(m_labPath)));
            return;
        }

        QTextStream labIn(&labFile);
        if (m_g2p) {
            const auto g2pRes = m_g2p->hanziToPinyin(asrMsg, Pinyin::ManTone::NORMAL, Pinyin::Error::Default, true);
            asrMsg = g2pRes.toStdStr().c_str();
        }

        labIn << QString::fromStdString(asrMsg);
        labFile.close();
        Q_EMIT this->oneFinished(m_filename, QString::fromStdString(asrMsg));
    }
}