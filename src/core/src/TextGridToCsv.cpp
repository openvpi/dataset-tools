#include <dstools/TextGridToCsv.h>

#include <textgrid.hpp>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include <memory>
#include <sstream>

namespace dstools {

bool TextGridToCsv::extractFromTextGrid(const QString &tgPath,
                                         TranscriptionRow &row,
                                         QString &error) {
    QFile file(tgPath);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Cannot open file: %1").arg(tgPath);
        return false;
    }

    const QByteArray bytes = file.readAll();
    file.close();

    const std::string content(bytes.constData(), bytes.size());
    std::istringstream iss(content);

    textgrid::TextGrid tg;
    try {
        textgrid::Parser parser(iss);
        tg = parser.Parse();
    } catch (const textgrid::Exception &e) {
        error = QStringLiteral("TextGrid parse error in %1: %2")
                    .arg(tgPath, QString::fromStdString(e.what()));
        return false;
    }

    // Find "phones" tier (case-insensitive)
    std::shared_ptr<textgrid::IntervalTier> phonesTier;
    for (size_t i = 0; i < tg.GetNumberOfTiers(); ++i) {
        auto tier = tg.GetTier(i);
        if (QString::fromStdString(tier->GetName())
                .compare(QStringLiteral("phones"), Qt::CaseInsensitive) == 0) {
            phonesTier = std::dynamic_pointer_cast<textgrid::IntervalTier>(tier);
            break;
        }
    }

    if (!phonesTier) {
        error = QStringLiteral("No \"phones\" interval tier found in %1").arg(tgPath);
        return false;
    }

    QStringList phSeqList;
    QStringList phDurList;

    for (size_t i = 0; i < phonesTier->GetNumberOfIntervals(); ++i) {
        const auto &interval = phonesTier->GetInterval(i);
        const QString text = QString::fromStdString(interval.text).trimmed();
        if (text.isEmpty())
            continue;

        const double duration = interval.max_time - interval.min_time;
        phSeqList.append(text);
        phDurList.append(QString::number(duration, 'f', 6));
    }

    row.name = QFileInfo(tgPath).completeBaseName();
    row.phSeq = phSeqList.join(QChar(' '));
    row.phDur = phDurList.join(QChar(' '));

    return true;
}

bool TextGridToCsv::extractDirectory(const QString &dir,
                                      std::vector<TranscriptionRow> &rows,
                                      QString &error) {
    QDir d(dir);
    if (!d.exists()) {
        error = QStringLiteral("Directory does not exist: %1").arg(dir);
        return false;
    }

    const QStringList files =
        d.entryList({QStringLiteral("*.TextGrid")}, QDir::Files | QDir::Readable, QDir::Name);

    for (const QString &filename : files) {
        const QString filePath = d.filePath(filename);
        TranscriptionRow row;
        if (!extractFromTextGrid(filePath, row, error)) {
            error = QStringLiteral("Failed on %1: %2").arg(filename, error);
            return false;
        }
        rows.push_back(std::move(row));
    }

    return true;
}

} // namespace dstools
