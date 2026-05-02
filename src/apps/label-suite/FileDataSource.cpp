#include "FileDataSource.h"

#include <QFileInfo>

namespace dstools {

FileDataSource::FileDataSource(QObject *parent) : IEditorDataSource(parent) {}

void FileDataSource::setFile(const QString &audioFilePath,
                              const QString &annotationPath) {
    m_audioPath = audioFilePath;
    m_annotationPath = annotationPath;
    emit sliceListChanged();
}

void FileDataSource::clear() {
    m_audioPath.clear();
    m_annotationPath.clear();
    emit sliceListChanged();
}

bool FileDataSource::hasFile() const {
    return !m_annotationPath.isEmpty();
}

QStringList FileDataSource::sliceIds() const {
    if (m_annotationPath.isEmpty())
        return {};
    return {m_annotationPath};
}

Result<DsTextDocument> FileDataSource::loadSlice(const QString &sliceId) {
    Q_UNUSED(sliceId)
    return DsTextDocument::load(m_annotationPath);
}

Result<void> FileDataSource::saveSlice(const QString &sliceId,
                                        const DsTextDocument &doc) {
    Q_UNUSED(sliceId)
    auto result = doc.save(m_annotationPath);
    if (result)
        emit sliceSaved(m_annotationPath);
    return result;
}

QString FileDataSource::audioPath(const QString &sliceId) const {
    Q_UNUSED(sliceId)
    return m_audioPath;
}

} // namespace dstools
