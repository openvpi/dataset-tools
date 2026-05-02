#pragma once

#include <QObject>

#include <memory>

namespace dstools {
namespace pitchlabeler {

class DSFile;

/// Manages file I/O operations (load, save, reload) for PitchLabeler.
/// Owns the current file state and emits signals when it changes.
class PitchFileService : public QObject {
    Q_OBJECT

public:
    explicit PitchFileService(QObject *parent = nullptr);
    ~PitchFileService() override;

    /// Load a .ds file from the given path.
    /// @return true if the file was loaded successfully.
    bool loadFile(const QString &path);

    /// Save the currently loaded file.
    /// @return true on success.
    bool saveFile();

    /// Save all modified files (currently delegates to saveFile).
    /// @return true on success.
    bool saveAllFiles();

    /// Reload the current file from disk.
    void reloadCurrentFile();

    /// @return the currently loaded DSFile, or nullptr.
    [[nodiscard]] std::shared_ptr<DSFile> currentFile() const { return m_currentFile; }

    /// @return the path of the currently loaded file.
    [[nodiscard]] QString currentFilePath() const { return m_currentFilePath; }

    /// @return true if the current file has unsaved modifications.
    [[nodiscard]] bool hasUnsavedChanges() const;

    /// Mark the current file as modified externally (e.g. after an edit).
    void markCurrentFileModified();

signals:
    /// Emitted after a file is successfully loaded.
    void fileLoaded(const QString &path, std::shared_ptr<DSFile> file);

    /// Emitted after a file is successfully saved.
    void fileSaved(const QString &path);

    /// Emitted when a file operation fails.
    void fileError(const QString &title, const QString &message);

    /// Emitted when the modification state changes.
    void modificationChanged(bool modified);

private:
    QString m_currentFilePath;
    std::shared_ptr<DSFile> m_currentFile;
};

} // namespace pitchlabeler
} // namespace dstools
