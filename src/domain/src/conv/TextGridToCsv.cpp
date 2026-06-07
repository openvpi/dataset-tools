#include <dstools/TextGridToCsv.h>

#include <dsfw/PathUtils.h>
#include <textgrid.hpp>

#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include <memory>
#include <sstream>

namespace dstools {

using namespace dsfw;

Result<TranscriptionRow> TextGridToCsv::extractFromTextGrid(const QString &tgPath) {
    auto textResult = dsfw::PathUtils::readFile(tgPath);
    if (!textResult.ok()) {
        return Result<TranscriptionRow>::Error(textResult.error());
    }

    const std::string content = textResult.value().toStdString();
    std::istringstream iss(content);

    textgrid::TextGrid tg;
    try {
        textgrid::Parser parser(iss);
        tg = parser.Parse();
    } catch (const textgrid::Exception &e) {
        return Result<TranscriptionRow>::Error(
            std::string("TextGrid parse error in ") + tgPath.toStdString() + ": " + e.what());
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
        return Result<TranscriptionRow>::Error(
            std::string("No \"phones\" interval tier found in ") + tgPath.toStdString());
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

    TranscriptionRow row;
    row.name = QFileInfo(tgPath).completeBaseName();
    row.phSeq = phSeqList.join(QChar(' '));
    row.phDur = phDurList.join(QChar(' '));

    return row;
}

Result<std::vector<TranscriptionRow>> TextGridToCsv::extractDirectory(const QString &dir) {
    QDir d(dir);
    if (!d.exists()) {
        return Result<std::vector<TranscriptionRow>>::Error(
            std::string("Directory does not exist: ") + dir.toStdString());
    }

    const QStringList files =
        d.entryList({QStringLiteral("*.TextGrid")}, QDir::Files | QDir::Readable, QDir::Name);

    std::vector<TranscriptionRow> rows;
    for (const QString &filename : files) {
        const QString filePath = d.filePath(filename);
        auto rowResult = extractFromTextGrid(filePath);
        if (!rowResult.ok()) {
            return Result<std::vector<TranscriptionRow>>::Error(
                std::string("Failed on ") + filename.toStdString() + ": " + rowResult.error());
        }
        rows.push_back(std::move(rowResult.value()));
    }

    return rows;
}

} // namespace dstools