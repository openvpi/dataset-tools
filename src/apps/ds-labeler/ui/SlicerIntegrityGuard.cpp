#include "SlicerIntegrityGuard.h"

#include <dstools/DsKeys.h>
#include <dstools/DsTextTypes.h>
#include <dstools/ProjectPaths.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace dstools {


    std::vector<FileIntegrityReport> SlicerIntegrityGuard::scanAffectedSlices(
        const QString &workingDir, const std::vector<QString> &affectedBaseNames) {
        std::vector<FileIntegrityReport> reports;
        const QString dsitemDir = ProjectPaths::dsItemsDir(workingDir);

        for (const auto &baseName : affectedBaseNames) {
            FileIntegrityReport fileReport;
            fileReport.baseName = baseName;

            QStringList dsitemFiles = findDsitemFiles(dsitemDir, baseName);
            for (const auto &dsitemFile : dsitemFiles) {
                QString sliceId = QFileInfo(dsitemFile).completeBaseName();
                QString dstextPath = ProjectPaths::sliceDstextPath(workingDir, sliceId);

                if (!QFile::exists(dstextPath))
                    continue;

                auto loadResult = DsTextDocument::load(dstextPath);
                if (!loadResult)
                    continue;

                const auto &doc = loadResult.value();
                SliceIntegrityReport sliceReport;
                sliceReport.sliceId = sliceId;
                sliceReport.dstextPath = dstextPath;

                for (const auto &layer : doc.layers) {
                    if (layer.boundaries.empty())
                        continue;

                    bool hasAnnotatedText = false;
                    for (const auto &b : layer.boundaries) {
                        if (!b.text.isEmpty()) {
                            hasAnnotatedText = true;
                            break;
                        }
                    }

                    if (hasAnnotatedText) {
                        SliceIntegrityReport::LayerInfo info;
                        info.name = layer.name;
                        info.boundaryCount = static_cast<int>(layer.boundaries.size());
                        sliceReport.annotatedLayers.push_back(info);
                    }
                }

                for (const auto &curve : doc.curves) {
                    if (curve.values.empty())
                        continue;

                    SliceIntegrityReport::LayerInfo info;
                    info.name = curve.name;
                    info.valueCount = static_cast<int>(curve.values.size());
                    sliceReport.annotatedLayers.push_back(info);
                }

                if (!sliceReport.annotatedLayers.empty())
                    fileReport.slices.push_back(std::move(sliceReport));
            }

            reports.push_back(std::move(fileReport));
        }

        return reports;
    }

    bool SlicerIntegrityGuard::backupSlice(const QString &dstextPath, QString &backupPath) {
        backupPath = makeBackupPath(dstextPath, QDateTime::currentDateTime());
        return QFile::copy(dstextPath, backupPath);
    }

    bool SlicerIntegrityGuard::restoreBackup(const QString &backupPath, const QString &originalPath) {
        if (!QFile::exists(backupPath))
            return false;
        return QFile::copy(backupPath, originalPath);
    }

    QString SlicerIntegrityGuard::makeBackupPath(const QString &originalPath, const QDateTime &timestamp) {
        QString suffix = QStringLiteral(".bak.") + timestamp.toString(QStringLiteral("yyyyMMdd_HHmmss"));
        return originalPath + suffix;
    }

    QString SlicerIntegrityGuard::dataLossSummary(const FileIntegrityReport &report) const {
        int totalGrapheme = 0;
        int totalPhoneme = 0;
        int totalPitch = 0;
        int totalMidi = 0;
        int totalSlices = 0;

        for (const auto &slice : report.slices) {
            if (slice.annotatedLayers.empty())
                continue;
            ++totalSlices;
            for (const auto &layer : slice.annotatedLayers) {
                if (layer.name == QString::fromUtf8(keys::layers::grapheme))
                    totalGrapheme += layer.boundaryCount;
                else if (layer.name.contains(QString::fromUtf8(keys::layers::phoneme), Qt::CaseInsensitive))
                    totalPhoneme += layer.boundaryCount;
                else if (layer.name == QString::fromUtf8(keys::layers::pitch))
                    totalPitch += layer.valueCount;
                else if (layer.name == QString::fromUtf8(keys::layers::midi))
                    totalMidi += layer.boundaryCount;
            }
        }

        QStringList parts;
        if (totalGrapheme > 0)
            parts.append(QStringLiteral("%1 个歌词标注").arg(totalGrapheme));
        if (totalPhoneme > 0)
            parts.append(QStringLiteral("%1 个音素对齐").arg(totalPhoneme));
        if (totalPitch > 0)
            parts.append(QStringLiteral("%1 个音高点").arg(totalPitch));
        if (totalMidi > 0)
            parts.append(QStringLiteral("%1 个MIDI音符").arg(totalMidi));

        if (parts.isEmpty())
            return QStringLiteral("无标注数据");

        return parts.join(QStringLiteral("、")) + QStringLiteral("（共 %1 个切片）将丢失").arg(totalSlices);
    }

    QString SlicerIntegrityGuard::exportedStatePath(const QString &workingDir) {
        return QDir(workingDir).filePath(QStringLiteral(".slicer_exported_state.json"));
    }

    void SlicerIntegrityGuard::recordExportedSlicePoints(
        const QString &workingDir, const std::map<QString, std::vector<double>> &slicePoints) {
        QJsonObject root;
        for (const auto &[filePath, points] : slicePoints) {
            QJsonArray arr;
            for (double p : points)
                arr.append(p);
            root[filePath] = arr;
        }
        QJsonDocument doc(root);
        QFile file(exportedStatePath(workingDir));
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(doc.toJson(QJsonDocument::Compact));
        }
    }

    std::vector<SliceExportConsistency> SlicerIntegrityGuard::checkSliceExportConsistency(
        const QString &workingDir,
        const std::map<QString, std::vector<double>> &currentSlicePoints) const {
        std::vector<SliceExportConsistency> results;

        QString statePath = exportedStatePath(workingDir);
        if (!QFile::exists(statePath))
            return results;

        QFile file(statePath);
        if (!file.open(QIODevice::ReadOnly))
            return results;

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject())
            return results;

        QJsonObject root = doc.object();

        for (const auto &[filePath, currentPoints] : currentSlicePoints) {
            if (currentPoints.empty())
                continue;

            SliceExportConsistency entry;
            entry.filePath = filePath;
            entry.baseName = QFileInfo(filePath).completeBaseName();
            entry.currentPoints = currentPoints;

            if (root.contains(filePath)) {
                entry.hasExportedItems = true;
                QJsonArray arr = root[filePath].toArray();
                for (const auto &v : arr)
                    entry.exportedPoints.push_back(v.toDouble());
            }

            if (entry.needsReExport())
                results.push_back(std::move(entry));
        }

        return results;
    }

    QStringList SlicerIntegrityGuard::findDsitemFiles(const QString &dsitemDir, const QString &baseName) const {
        QDir dir(dsitemDir);
        return dir.entryList({baseName + QStringLiteral("*.dsitem")}, QDir::Files);
    }

} // namespace dstools
