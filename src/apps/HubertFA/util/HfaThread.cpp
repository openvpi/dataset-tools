#include "HfaThread.h"

#include <QMessageBox>

#include "textgrid.hpp"

#include <QFileInfo>
#include <sstream>

namespace HFA {
    HfaThread::HfaThread(HFA *hfa, QString filename, const QString &wavPath, const QString &outTgPath,
                         const std::string &language, const std::vector<std::string> &non_speech_ph)
        : m_hfa(hfa), m_filename(std::move(filename)), m_wavPath(wavPath), m_outTgPath(outTgPath), m_language(language),
          m_non_speech_ph(non_speech_ph) {
    }

    void HfaThread::run() {
        std::string fblMsg;
        try {
            HfaLogits hfaRes;
            if (!QFile(m_wavPath).exists()) {
                fblMsg = "File does not exist: " + m_wavPath.toLocal8Bit().toStdString();
                Q_EMIT this->oneFailed(m_filename, QString::fromStdString(fblMsg));
                return;
            }

            WordList words;
            if (m_hfa->recognize(m_wavPath.toLocal8Bit().toStdString(), m_language, m_non_speech_ph, words, fblMsg)) {
                if (words.empty()) {
                    fblMsg = "No words recognized in audio: " + m_wavPath.toLocal8Bit().toStdString() + "-" + fblMsg;
                    Q_EMIT this->oneFailed(m_filename, QString::fromStdString(fblMsg));
                    return;
                }

                // Create TextGrid
                textgrid::TextGrid outTg(0.0, words.duration());

                auto tierWords = std::make_shared<textgrid::IntervalTier>("words", 0.0, words.duration());
                auto tierPhones = std::make_shared<textgrid::IntervalTier>("phonemes", 0.0, words.duration());

                for (auto word : words) {
                    tierWords->AppendInterval(textgrid::Interval(std::max(0.0f, word->start), word->end, word->text));
                    for (auto phoneme : word->phonemes) {
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
                    fblMsg = "Failed to open file for writing.";
                    Q_EMIT this->oneFailed(m_filename, QString::fromStdString(fblMsg));
                    return;
                }
            } else {
                fblMsg = "Failed to recognize audio: " + m_wavPath.toStdString() + " - " + fblMsg;
                Q_EMIT this->oneFailed(m_filename, QString::fromStdString(fblMsg));
                return;
            }
        } catch (const std::exception &e) {
            Q_EMIT this->oneFailed(m_filename, "Exception: " + QString::fromStdString(e.what()));
            return;
        }
        fblMsg = "success.";
        Q_EMIT this->oneFinished(m_filename, QString::fromStdString(fblMsg));
    }
}