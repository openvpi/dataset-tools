/// @file IEditorDataSource.h
/// @brief Abstract data source interface for editor components.
///
/// Decouples core editors (MinLabelEditor, PhonemeEditor, PitchEditor) from
/// concrete storage backends. LabelSuite uses FileDataSource (file system);
/// DsLabeler uses ProjectDataSource (.dstext + PipelineContext).

#pragma once

#include <dstools/DsTextTypes.h>
#include <dstools/Result.h>

#include <QFile>
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
///
/// Thread safety (P-11):
/// - `sliceIds()`, `audioPath()`, `validatedAudioPath()`, `audioExists()`,
///   `sliceDuration()`, `dirtyLayers()` are const and may be called from
///   background threads as long as no concurrent mutation occurs.
/// - `loadSlice()` is read-only in practice but not marked const (some
///   implementations may cache). Callers must ensure no concurrent write.
/// - `saveSlice()`, `clearDirtyLayers()` are mutating and must only be
///   called from the main thread.
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

    /// Check whether the audio file for a slice exists on disk.
    /// Default: QFile::exists(audioPath(sliceId)).
    [[nodiscard]] virtual bool audioExists(const QString &sliceId) const;

    /// Return a validated audio path: non-empty only if the file exists.
    /// Use this instead of raw audioPath() before passing to inference engines.
    [[nodiscard]] QString validatedAudioPath(const QString &sliceId) const;

    /// Return all slice IDs whose audio files are missing.
    [[nodiscard]] QStringList missingAudioSlices() const;

    /// Return the duration of a slice in seconds, or -1.0 if unknown.
    /// Default implementation returns -1.0.
    [[nodiscard]] virtual double sliceDuration(const QString &sliceId) const {
        Q_UNUSED(sliceId)
        return -1.0;
    }

    /// Return dirty layer names for the given slice (pipeline dependency tracking).
    /// Default implementation returns an empty list (no dirty layers).
    [[nodiscard]] virtual QStringList dirtyLayers(const QString &sliceId) const {
        Q_UNUSED(sliceId)
        return {};
    }

    /// Clear the dirty flag for the specified layers of the given slice.
    /// Default implementation does nothing.
    virtual void clearDirtyLayers(const QString &sliceId, const QStringList &layers) {
        Q_UNUSED(sliceId)
        Q_UNUSED(layers)
    }

signals:
    /// Emitted when the underlying slice list changes (e.g. project reload).
    void sliceListChanged();

    /// Emitted after a slice is successfully saved.
    void sliceSaved(const QString &sliceId);

    /// Emitted when audio file availability changes (files appear/disappear).
    void audioAvailabilityChanged();
};

} // namespace dstools
