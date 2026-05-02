#include "AsrTask.h"

#include <QApplication>
#include <QFile>

namespace LyricFA {
    AsrThread::AsrThread(Asr *asr, QString filename, QString wavPath, QString labPath,
                         const QSharedPointer<Pinyin::Pinyin> &g2p)
        : AsyncTask(std::move(filename)), m_asr(asr), m_wavPath(std::move(wavPath)), m_labPath(std::move(labPath)),
          m_g2p(g2p) {
    }

    bool AsrThread::execute(QString &msg) {
        std::string asrMsg;

        if (const auto asrRes = m_asr->recognize(m_wavPath.toLocal8Bit().toStdString(), asrMsg); !asrRes) {
            msg = QString::fromStdString(asrMsg);
            return false;
        }

        QFile labFile(m_labPath);
        if (!labFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            msg = QString("Failed to write to file %1").arg(m_labPath);
            return false;
        }

        QTextStream labIn(&labFile);
        if (m_g2p != nullptr) {
            const auto g2pRes = m_g2p->hanziToPinyin(asrMsg, Pinyin::ManTone::NORMAL, Pinyin::Error::Default, true);
            asrMsg = g2pRes.toStdStr();
        }

        labIn << QString::fromStdString(asrMsg);
        labFile.close();
        msg = QString::fromStdString(asrMsg);
        return true;
    }
}
