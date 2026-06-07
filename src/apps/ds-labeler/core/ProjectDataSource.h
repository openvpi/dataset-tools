/// @file ProjectDataSource.h
/// @brief IEditorDataSource implementation for .dsproj project-backed editing.

#pragma once

#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <dsfw/PipelineContext.h>
#include <dstools/DsProject.h>
#include <dstools/IEditorDataSource.h>
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
        [[nodiscard]] DsProject *project() const {
            return m_project;
        }

        /// Access the working directory.
        [[nodiscard]] const QString &workingDir() const {
            return m_workingDir;
        }

        /// Access/load PipelineContext for a slice (lazy-loaded, cached).
        [[nodiscard]] dsfw::PipelineContext *context(const QString &sliceId);

        /// Save PipelineContext for a slice to disk.
        [[nodiscard]] dsfw::Result<void> saveContext(const QString &sliceId);

        /// Re-scan audio file availability and emit audioAvailabilityChanged if changed.
        void rescanAudioAvailability();

        // IEditorDataSource
        [[nodiscard]] int getSliceCount() const override;
        [[nodiscard]] QStringList sliceIds() const override;
        [[nodiscard]] dsfw::Result<DsTextDocument> loadSlice(const QString &sliceId) override;
        [[nodiscard]] dsfw::Result<void> saveSlice(const QString &sliceId, const DsTextDocument &doc) override;
        [[nodiscard]] QString audioPath(const QString &sliceId) const override;
        [[nodiscard]] bool audioExists(const QString &sliceId) const override;
        [[nodiscard]] double sliceDuration(const QString &sliceId) const override;
        [[nodiscard]] QStringList dirtyLayers(const QString &sliceId) const override;
        void clearDirtyLayers(const QString &sliceId, const QStringList &layers) override;
        void addDirtyLayers(const QString &sliceId, const QStringList &modifiedLayers) override;
        void setLayerManuallyEdited(const QString &sliceId, const QString &layer, bool edited) override;
        [[nodiscard]] bool isLayerManuallyEdited(const QString &sliceId, const QString &layer) const override;
        void addEditedStep(const QString &sliceId, const QString &step) override;

    private:
        DsProject *m_project = nullptr;
        QString m_workingDir;
        std::map<QString, dsfw::PipelineContext> m_contexts;
        mutable QMutex m_contextsMutex;

        mutable QSet<QString> m_missingAudioIds;
        mutable bool m_audioScanned = false;

        void ensureAudioScanned() const;

        [[nodiscard]] QString contextPath(const QString &sliceId) const;
        [[nodiscard]] QString dstextPath(const QString &sliceId) const;
        [[nodiscard]] QString sliceAudioPath(const QString &sliceId) const;
        [[nodiscard]] const Slice *findSlice(const QString &sliceId) const;
    };

} // namespace dstools
