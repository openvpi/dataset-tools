#include "PitchFileService.h"

#include <DSFile.h>

namespace dstools {
namespace pitchlabeler {

    PitchFileService::PitchFileService(QObject *parent)
        : QObject(parent) {
    }

    PitchFileService::~PitchFileService() = default;

    bool PitchFileService::loadFile(const QString &path) {
        auto [ds, error] = DSFile::load(path);
        if (!error.isEmpty()) {
            emit fileError(QStringLiteral("错误"),
                           QStringLiteral("加载文件失败: ") + error);
            return false;
        }

        m_currentFile = ds;
        m_currentFilePath = path;

        emit fileLoaded(path, ds);
        return true;
    }

    bool PitchFileService::saveFile() {
        if (!m_currentFile)
            return false;

        auto [ok, error] = m_currentFile->save();
        if (!ok) {
            emit fileError(QStringLiteral("错误"),
                           QStringLiteral("保存失败: ") + error);
            return false;
        }

        emit fileSaved(m_currentFilePath);
        emit modificationChanged(false);
        return true;
    }

    bool PitchFileService::saveAllFiles() {
        if (m_currentFile && m_currentFile->modified) {
            return saveFile();
        }
        return true;
    }

    void PitchFileService::reloadCurrentFile() {
        if (!m_currentFilePath.isEmpty()) {
            loadFile(m_currentFilePath);
        }
    }

    bool PitchFileService::hasUnsavedChanges() const {
        return m_currentFile && m_currentFile->modified;
    }

    void PitchFileService::markCurrentFileModified() {
        if (m_currentFile) {
            m_currentFile->markModified();
            emit modificationChanged(true);
        }
    }

} // namespace pitchlabeler
} // namespace dstools
