/// @file IEditorDataSource.h
/// @brief Abstract data source interface for editor components.
///
/// Decouples core editors (MinLabelEditor, PhonemeEditor, PitchEditor) from
/// concrete storage backends. LabelSuite uses FileDataSource (file system);
/// DsLabeler uses ProjectDataSource (.dstext + PipelineContext).

#pragma once

#include <dstools/DsTextTypes.h>
#include <dstools/Result.h>

#include <QObject>
#include <QString>
#include <QStringList>

namespace dstools {

/// @brief Abstract data source for shared editor widgets (ADR-52).
///
/// Implementations provide slice enumeration, load/save, and audio path
/// resolution. Editors interact exclusively through this interface so that
/// the same widget code can serve both LabelSuite (single-file mode) and
/// DsLabeler (project mode).
class IEditorDataSource : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IEditorDataSource() override = default;

    /// Return the list of available slice identifiers.
    /// For FileDataSource a single file maps to a single slice.
    [[nodiscard]] virtual QStringList sliceIds() const = 0;

    /// Load annotation data for the given slice.
    [[nodiscard]] virtual Result<DsTextDocument> loadSlice(const QString &sliceId) = 0;

    /// Persist annotation data for the given slice.
    [[nodiscard]] virtual Result<void> saveSlice(const QString &sliceId,
                                                 const DsTextDocument &doc) = 0;

    /// Resolve the audio file path associated with a slice.
    [[nodiscard]] virtual QString audioPath(const QString &sliceId) const = 0;

signals:
    /// Emitted when the underlying slice list changes (e.g. project reload).
    void sliceListChanged();

    /// Emitted after a slice is successfully saved.
    void sliceSaved(const QString &sliceId);
};

} // namespace dstools
