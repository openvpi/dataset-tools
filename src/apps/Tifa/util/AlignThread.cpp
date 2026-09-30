#include "AlignThread.h"

#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "PathUtils.h"

namespace Tifa {

    namespace {

        /** TIFA reads the transcript next to the audio, preferring ``.txt``. */
        QString findTextFile(const QString &wavPath) {
            const QFileInfo info(wavPath);
            const QString base = info.dir().filePath(info.completeBaseName());
            const QString textPath = base + ".txt";
            if (QFile::exists(textPath)) {
                return textPath;
            }
            const QString labPath = base + ".lab";
            return QFile::exists(labPath) ? labPath : QString();
        }

    } // namespace

    AlignThread::AlignThread(const Aligner *aligner, QString filename, QString wavPath, QString outTgPath,
                             AlignRequest request)
        : m_aligner(aligner), m_filename(std::move(filename)), m_wavPath(std::move(wavPath)),
          m_outTgPath(std::move(outTgPath)), m_request(std::move(request)) {
    }

    void AlignThread::run() {
        if (!QFile::exists(m_wavPath)) {
            Q_EMIT oneFailed(m_filename, "File does not exist: " + m_wavPath);
            return;
        }

        const QString textPath = findTextFile(m_wavPath);
        if (textPath.isEmpty()) {
            Q_EMIT oneFailed(m_filename, "No paired text file next to " + QFileInfo(m_wavPath).fileName());
            return;
        }

        QFile textFile(textPath);
        if (!textFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            Q_EMIT oneFailed(m_filename, "Failed to open " + textPath);
            return;
        }
        const QString text = QString::fromUtf8(textFile.readAll()).trimmed();
        textFile.close();
        if (text.isEmpty()) {
            Q_EMIT oneFailed(m_filename, "Empty text: " + textPath);
            return;
        }

        AlignRequest request = m_request;
        request.wavPath = toPath(m_wavPath);
        request.text = text.toStdString();

        AlignResult alignResult;
        std::string error;
        if (!m_aligner->align(request, alignResult, error)) {
            Q_EMIT oneFailed(m_filename, QString::fromStdString(error));
            return;
        }

        QString message = QString("Aligned %1 word(s) and %2 phone(s), agreement %3")
                              .arg(alignResult.words)
                              .arg(alignResult.phones)
                              .arg(alignResult.agreement, 0, 'f', 3);
        for (const std::string &warning : alignResult.warnings) {
            message += "\n  " + QString::fromStdString(warning);
        }
        message += "\n" + QString::fromStdString(alignResult.report).trimmed();

        if (alignResult.textGrid.empty()) {
            Q_EMIT oneFailed(m_filename, "No TextGrid written. " + message);
            return;
        }

        QFile outFile(m_outTgPath);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Q_EMIT oneFailed(m_filename, "Failed to open " + m_outTgPath + " for writing");
            return;
        }
        QTextStream stream(&outFile);
        stream << QString::fromUtf8(alignResult.textGrid.data(),
                                    static_cast<qsizetype>(alignResult.textGrid.size()));
        outFile.close();

        Q_EMIT oneFinished(m_filename, message);
    }

} // namespace Tifa
