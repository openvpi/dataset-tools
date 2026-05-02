/// @file FileDataSource.h
/// @brief IEditorDataSource implementation for file-system backed editing.

#pragma once

#include <dstools/IEditorDataSource.h>

namespace dstools {

/// @brief File-system backed data source for LabelSuite pages (ADR-52).
///
/// Maps a single annotation file to a single slice. Used by LabelSuite
/// pages where each file is an independent editing unit with no project context.
class FileDataSource : public IEditorDataSource {
    Q_OBJECT

public:
    explicit FileDataSource(QObject *parent = nullptr);
    ~FileDataSource() override = default;

    /// Set the current file pair (audio + annotation).
    void setFile(const QString &audioFilePath, const QString &annotationPath);

    /// Clear the current file.
    void clear();

    /// Check if a file is currently loaded.
    [[nodiscard]] bool hasFile() const;

    /// Return the annotation file path.
    [[nodiscard]] const QString &annotationPath() const { return m_annotationPath; }

    // IEditorDataSource
    [[nodiscard]] QStringList sliceIds() const override;
    [[nodiscard]] Result<DsTextDocument> loadSlice(const QString &sliceId) override;
    [[nodiscard]] Result<void> saveSlice(const QString &sliceId,
                                         const DsTextDocument &doc) override;
    [[nodiscard]] QString audioPath(const QString &sliceId) const override;

private:
    QString m_audioPath;
    QString m_annotationPath;
};

} // namespace dstools
