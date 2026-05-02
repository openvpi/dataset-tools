/// @file Common.h
/// @brief Common utilities and data types for the MinLabel application.

#pragma once
#include <QFileInfo>
#include <QString>
#include <utility>

#include <nlohmann/json.hpp>

namespace Minlabel {
    /// @brief Replaces the suffix of an audio filename with the given target suffix.
    /// @param filename Original audio filename.
    /// @param tarSuffix Target suffix to apply.
    /// @return Filename with the new suffix.
    QString audioToOtherSuffix(const QString &filename, const QString &tarSuffix);

    /// @brief Finds the corresponding audio file for a label file.
    /// @param filename Label filename.
    /// @return Path to the matching audio file.
    QString labFileToAudioFile(const QString &filename);

    /// @brief Export settings for dataset output.
    struct ExportInfo {
        QString outputDir;   ///< Output directory path.
        QString folderName;  ///< Subfolder name for export.
        bool exportAudio;    ///< Whether to export audio files.
        bool labFile;        ///< Whether to export label files.
        bool rawText;        ///< Whether to export raw text files.
        bool removeTone;     ///< Whether to remove tone markers.
    };

    /// @brief File copy metadata for export operations.
    struct CopyInfo {
        QString rawName;     ///< Original filename.
        QString tarName;     ///< Target filename.
        QString sourceDir;   ///< Source directory.
        QString targetDir;   ///< Target directory.
        QString tarBasename; ///< Target filename without extension.
        bool exist;          ///< Whether the source file exists.

        /// @brief Constructs a CopyInfo entry.
        /// @param rawName Original filename.
        /// @param tarName Target filename.
        /// @param sourceDir Source directory.
        /// @param targetDir Target directory.
        /// @param isExist Whether the source file exists.
        CopyInfo(QString rawName, const QString &tarName, QString sourceDir, QString targetDir, bool isExist)
            : rawName(std::move(rawName)), tarName(tarName), sourceDir(std::move(sourceDir)),
              targetDir(std::move(targetDir)), exist(isExist) {
            const QString filename = QFileInfo(tarName).fileName();
            const QString suffix = QFileInfo(tarName).suffix();
            tarBasename = filename.mid(0, filename.size() - suffix.size() - 1);
        }
    };

    /// @brief Copies files according to the copy list and export settings.
    /// @param copyList List of files to copy.
    /// @param exportInfo Export configuration.
    /// @return True on success.
    bool copyFile(QList<CopyInfo> &copyList, const ExportInfo &exportInfo);

    /// @brief Counts JSON files in a directory.
    /// @param dirName Directory to scan.
    /// @return Number of JSON files found.
    int jsonCount(const QString &dirName);

    /// @brief Creates the output directory structure for export.
    /// @param exportInfo Export configuration containing directory paths.
    void mkdir(const ExportInfo &exportInfo);

    /// @brief Builds a copy list mapping source files to target paths.
    /// @param sourcePath Source directory.
    /// @param outputDir Output directory.
    /// @return List of CopyInfo entries.
    QList<CopyInfo> mkCopylist(const QString &sourcePath, const QString &outputDir);

    /// @brief Reads a JSON file into a json object.
    /// @param fileName Path to the JSON file.
    /// @param jsonData Output json object.
    /// @return True on success.
    bool readJsonFile(const QString &fileName, nlohmann::json &jsonData);

    /// @brief Writes a json object to a file.
    /// @param fileName Path to the output file.
    /// @param jsonData JSON data to write.
    /// @return True on success.
    bool writeJsonFile(const QString &fileName, const nlohmann::json &jsonData);
}
