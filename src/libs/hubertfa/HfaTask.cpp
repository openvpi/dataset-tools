#include "HfaTask.h"

#include <QMessageBox>

#include <textgrid.hpp>

#include <QFileInfo>
#include <sstream>

namespace HFA {
    HfaThread::HfaThread(HFA *hfa, QString filename, const QString &wavPath, const QString &outTgPath,
                         const std::string &language, const std::vector<std::string> &non_speech_ph)
        : AsyncTask(std::move(filename)), m_hfa(hfa), m_wavPath(wavPath), m_outTgPath(outTgPath),
          m_language(language), m_non_speech_ph(non_speech_ph) {
    }

    bool HfaThread::execute(QString &msg) {
        try {
            HfaLogits hfaRes;
            if (!QFile(m_wavPath).exists()) {
                msg = QString::fromStdString("File does not exist: " + m_wavPath.toLocal8Bit().toStdString());
                return false;
            }

            WordList words;
            auto recognizeResult = m_hfa->recognize(m_wavPath.toLocal8Bit().toStdString(), m_language, m_non_speech_ph, words);
            if (recognizeResult) {
                if (words.empty()) {
                    msg = QString::fromStdString("No words recognized in audio: " +
                                                 m_wavPath.toLocal8Bit().toStdString());
                    return false;
                }

                // Create TextGrid
                textgrid::TextGrid outTg(0.0, words.duration());

                auto tierWords = std::make_shared<textgrid::IntervalTier>("words", 0.0, words.duration());
                auto tierPhones = std::make_shared<textgrid::IntervalTier>("phones", 0.0, words.duration());

                for (auto word : words) {
                    tierWords->AppendInterval(textgrid::Interval(std::max(0.0f, word.start), word.end, word.text));
                    for (auto phoneme : word.phones) {
                        tierPhones->AppendInterval(
                            textgrid::Interval(std::max(0.0f, phoneme.start), phoneme.end, phoneme.text));
                    }
                }

                outTg.AppendTier(tierWords);
                outTg.AppendTier(tierPhones);

                QFile outFile(m_outTgPath);
                if (outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream outTgStream(&outFile);
                    std::ostringstream oss;
                    oss << outTg;
                    outTgStream << QString::fromStdString(oss.str());
                } else {
                    msg = QStringLiteral("Failed to open file for writing: %1 (%2)")
                              .arg(m_outTgPath, outFile.errorString());
                    return false;
                }
            } else {
                msg = QString::fromStdString("Failed to recognize audio: " + m_wavPath.toStdString() + " - " + recognizeResult.error());
                return false;
            }
        } catch (const std::exception &e) {
            msg = "Exception: " + QString::fromStdString(e.what());
            return false;
        }
        msg = QStringLiteral("success.");
        return true;
    }
}
