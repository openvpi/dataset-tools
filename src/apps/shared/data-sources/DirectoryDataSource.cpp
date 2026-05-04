#include "DirectoryDataSource.h"

#include <QDir>
#include <QFile>

#include <dstools/ProjectPaths.h>

namespace dstools {

DirectoryDataSource::DirectoryDataSource(QObject *parent)
    : IEditorDataSource(parent) {}

void DirectoryDataSource::setWorkingDirectory(const QString &dir) {
    if (m_workingDir == dir)
        return;
    m_workingDir = dir;
    refresh();
}

QString DirectoryDataSource::workingDirectory() const { return m_workingDir; }

void DirectoryDataSource::refresh() {
    m_sliceIds.clear();
    if (m_workingDir.isEmpty())
        return;

    const QString slicesDir = ProjectPaths::slicesDir(m_workingDir);
    QDir dir(slicesDir);
    if (!dir.exists())
        return;

    const QStringList audioFiles =
        dir.entryList({QStringLiteral("*.wav")}, QDir::Files, QDir::Name);
    for (const QString &fileName : audioFiles)
        m_sliceIds.append(QFileInfo(fileName).completeBaseName());

    emit sliceListChanged();
}

QStringList DirectoryDataSource::sliceIds() const { return m_sliceIds; }

Result<DsTextDocument> DirectoryDataSource::loadSlice(const QString &sliceId) {
    const QString path = dstextPath(sliceId);
    if (QFile::exists(path))
        return DsTextDocument::load(path);

    DsTextDocument doc;
    doc.audio.path = sliceAudioPath(sliceId);
    return doc;
}

Result<void> DirectoryDataSource::saveSlice(const QString &sliceId,
                                             const DsTextDocument &doc) {
    const QString path = dstextPath(sliceId);
    QDir().mkpath(QFileInfo(path).absolutePath());

    auto result = doc.save(path);
    if (result)
        emit sliceSaved(sliceId);
    return result;
}

QString DirectoryDataSource::audioPath(const QString &sliceId) const {
    return sliceAudioPath(sliceId);
}

QString DirectoryDataSource::dstextPath(const QString &sliceId) const {
    return ProjectPaths::sliceDstextPath(m_workingDir, sliceId);
}

QString DirectoryDataSource::sliceAudioPath(const QString &sliceId) const {
    return ProjectPaths::sliceAudioPath(m_workingDir, sliceId);
}

} // namespace dstools
