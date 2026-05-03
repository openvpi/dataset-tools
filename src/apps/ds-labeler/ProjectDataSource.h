/// @file ProjectDataSource.h
/// @brief IEditorDataSource implementation for .dsproj project-backed editing.

#pragma once

#include <dstools/IEditorDataSource.h>
#include <dstools/DsProject.h>
#include <dsfw/PipelineContext.h>

#include <map>

namespace dstools {

/// @brief Project-backed data source for DsLabeler pages (ADR-52).
///
/// Reads/writes .dstext documents and PipelineContext per slice.
/// Slices are enumerated from the DsProject's items list.
class ProjectDataSource : public IEditorDataSource {
    Q_OBJECT

public:
    explicit ProjectDataSource(QObject *parent = nullptr);
    ~ProjectDataSource() override = default;

    /// Load a project and build the slice list from items.
    void setProject(DsProject *project, const QString &workingDir);

    /// Clear project reference.
    void clear();

    /// Access the underlying project.
    [[nodiscard]] DsProject *project() const { return m_project; }

    /// Access the working directory.
    [[nodiscard]] const QString &workingDir() const { return m_workingDir; }

    /// Access/load PipelineContext for a slice (lazy-loaded, cached).
    [[nodiscard]] PipelineContext *context(const QString &sliceId);

    /// Save PipelineContext for a slice to disk.
    Result<void> saveContext(const QString &sliceId);

    // IEditorDataSource
    [[nodiscard]] QStringList sliceIds() const override;
    [[nodiscard]] Result<DsTextDocument> loadSlice(const QString &sliceId) override;
    [[nodiscard]] Result<void> saveSlice(const QString &sliceId,
                                         const DsTextDocument &doc) override;
    [[nodiscard]] QString audioPath(const QString &sliceId) const override;
    [[nodiscard]] QStringList dirtyLayers(const QString &sliceId) const override;
    void clearDirtyLayers(const QString &sliceId, const QStringList &layers) override;

private:
    DsProject *m_project = nullptr;
    QString m_workingDir;
    std::map<QString, PipelineContext> m_contexts;

    [[nodiscard]] QString contextPath(const QString &sliceId) const;
    [[nodiscard]] QString dstextPath(const QString &sliceId) const;
    [[nodiscard]] QString sliceAudioPath(const QString &sliceId) const;
    [[nodiscard]] const Slice *findSlice(const QString &sliceId) const;
};

} // namespace dstools
