#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <vector>

namespace dstools {

    struct SliceIntegrityReport {
        QString sliceId;
        QString dstextPath;
        struct LayerInfo {
            QString name;
            int boundaryCount = 0;
            int valueCount = 0;
        };
        std::vector<LayerInfo> annotatedLayers;
        bool hasBackup = false;
        QString backupPath;
    };

    struct FileIntegrityReport {
        QString filePath;
        QString baseName;
        std::vector<SliceIntegrityReport> slices;
        bool hasAnnotatedData() const {
            for (const auto &s : slices)
                if (!s.annotatedLayers.empty()) return true;
            return false;
        }
    };

    struct SliceExportConsistency {
        QString filePath;
        QString baseName;
        std::vector<double> currentPoints;
        std::vector<double> exportedPoints;
        bool hasExportedItems = false;

        bool needsReExport() const { return hasExportedItems && currentPoints != exportedPoints; }
    };

    class SlicerIntegrityGuard {
    public:
        SlicerIntegrityGuard() = default;

        std::vector<FileIntegrityReport> scanAffectedSlices(const QString &workingDir,
                                                            const std::vector<QString> &affectedBaseNames);

        bool backupSlice(const QString &dstextPath, QString &backupPath);

        bool restoreBackup(const QString &backupPath, const QString &originalPath);

        static QString makeBackupPath(const QString &originalPath, const QDateTime &timestamp);

        QString dataLossSummary(const FileIntegrityReport &report) const;

        void recordExportedSlicePoints(const QString &workingDir,
                                       const std::map<QString, std::vector<double>> &slicePoints);

        std::vector<SliceExportConsistency> checkSliceExportConsistency(
            const QString &workingDir,
            const std::map<QString, std::vector<double>> &currentSlicePoints) const;

        static QString exportedStatePath(const QString &workingDir);

    private:
        QStringList findDsitemFiles(const QString &dsitemDir, const QString &baseName) const;
    };

} // namespace dstools