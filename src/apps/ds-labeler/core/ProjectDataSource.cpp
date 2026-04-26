#include "ProjectDataSource.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QDebug>

#include <dsfw/AtomicFileWriter.h>
#include <dsfw/PathUtils.h>
#include <nlohmann/json.hpp>

#include <dstools/ProjectPaths.h>

namespace dstools {

ProjectDataSource::ProjectDataSource(QObject *parent) : IEditorDataSource(parent) {}

void ProjectDataSource::setProject(DsProject *project, const QString &workingDir) {
    QMutexLocker locker(&m_contextsMutex);
    
    m_project = project;
    m_workingDir = workingDir;
    m_contexts.clear();
    m_audioScanned = false;
    m_missingAudioIds.clear();
    emit sliceListChanged();
}

void ProjectDataSource::clear() {
    QMutexLocker locker(&m_contextsMutex);
    
    m_project = nullptr;
    m_workingDir.clear();
    m_contexts.clear();
    m_audioScanned = false;
    m_missingAudioIds.clear();
    emit sliceListChanged();
}

int ProjectDataSource::getSliceCount() const { return sliceIds().size(); }

QStringList ProjectDataSource::sliceIds() const {
    QStringList ids;
    if (!m_project)
        return ids;

    for (const auto &item : m_project->items()) {
        for (const auto &slice : item.slices) {
            if (slice.status == QStringLiteral("active"))
                ids.append(slice.id);
        }
    }
    return ids;
}

Result<DsTextDocument> ProjectDataSource::loadSlice(const QString &sliceId) {
    const QString path = dstextPath(sliceId);
    if (QFile::exists(path))
        return DsTextDocument::load(path);

    // Return empty document for slices without existing dstext
    DsTextDocument doc;
    doc.audio.path = sliceAudioPath(sliceId);
    return doc;
}

Result<void> ProjectDataSource::saveSlice(const QString &sliceId,
                                           const DsTextDocument &doc) {
    const QString path = dstextPath(sliceId);

    // Ensure directory exists
    QDir().mkpath(QFileInfo(path).absolutePath());

    auto result = doc.save(path);
    if (result)
        emit sliceSaved(sliceId);
    return result;
}

QString ProjectDataSource::audioPath(const QString &sliceId) const {
    return sliceAudioPath(sliceId);
}

double ProjectDataSource::sliceDuration(const QString &sliceId) const {
    const Slice *slice = findSlice(sliceId);
    if (!slice || slice->outPos <= slice->inPos)
        return -1.0;
    return static_cast<double>(slice->outPos - slice->inPos) / 1'000'000.0;
}

QStringList ProjectDataSource::dirtyLayers(const QString &sliceId) const {
    QMutexLocker locker(&m_contextsMutex);
    
    auto it = m_contexts.find(sliceId);
    if (it != m_contexts.end())
        return it->second.dirty;
    return {};
}

void ProjectDataSource::clearDirtyLayers(const QString &sliceId,
                                          const QStringList &layers) {
    QMutexLocker locker(&m_contextsMutex);
    
    auto it = m_contexts.find(sliceId);
    if (it == m_contexts.end())
        return;
    for (const auto &layer : layers)
        it->second.dirty.removeAll(layer);
    saveContext(sliceId);
}

void ProjectDataSource::addDirtyLayers(const QString &sliceId,
                                       const QStringList &modifiedLayers) {
    QMutexLocker locker(&m_contextsMutex);
    
    auto it = m_contexts.find(sliceId);
    if (it == m_contexts.end()) {
        // 如果上下文不存在，创建它
        PipelineContext ctx;
        ctx.itemId = sliceId;
        ctx.audioPath = sliceAudioPath(sliceId);
        it = m_contexts.insert({sliceId, std::move(ctx)}).first;
    }
    
    for (const auto &layer : modifiedLayers)
        it->second.propagateDirty(layer);
    
    // 保存上下文（saveContext内部会获取锁，但我们已经持有锁）
    // 为了避免死锁，我们直接在这里保存而不调用saveContext
    const QString path = contextPath(sliceId);

    auto j = it->second.toJson();
    auto str = j.dump(2);
    dsfw::AtomicFileWriter::write(dsfw::PathUtils::toStdPath(path), str);
}

void ProjectDataSource::setLayerManuallyEdited(const QString &sliceId,
                                               const QString &layer, bool edited) {
    QMutexLocker locker(&m_contextsMutex);

    auto it = m_contexts.find(sliceId);
    if (it == m_contexts.end())
        return;
    it->second.setLayerManuallyEdited(layer, edited);

    const QString path = contextPath(sliceId);

    auto j = it->second.toJson();
    auto str = j.dump(2);
    dsfw::AtomicFileWriter::write(dsfw::PathUtils::toStdPath(path), str);
}

bool ProjectDataSource::isLayerManuallyEdited(const QString &sliceId,
                                               const QString &layer) const {
    QMutexLocker locker(&m_contextsMutex);
    
    auto it = m_contexts.find(sliceId);
    if (it == m_contexts.end())
        return false;
    return it->second.isLayerManuallyEdited(layer);
}

void ProjectDataSource::addEditedStep(const QString &sliceId, const QString &step) {
    QMutexLocker locker(&m_contextsMutex);
    
    auto it = m_contexts.find(sliceId);
    if (it == m_contexts.end()) {
        // 如果上下文不存在，创建它
        PipelineContext ctx;
        ctx.itemId = sliceId;
        ctx.audioPath = sliceAudioPath(sliceId);
        it = m_contexts.insert({sliceId, std::move(ctx)}).first;
    }
    
    it->second.addEditedStep(step);

    const QString path = contextPath(sliceId);

    auto j = it->second.toJson();
    auto str = j.dump(2);
    dsfw::AtomicFileWriter::write(dsfw::PathUtils::toStdPath(path), str);
}

bool ProjectDataSource::audioExists(const QString &sliceId) const {
    ensureAudioScanned();
    return !m_missingAudioIds.contains(sliceId);
}

void ProjectDataSource::ensureAudioScanned() const {
    if (m_audioScanned)
        return;
    m_audioScanned = true;
    m_missingAudioIds.clear();
    if (!m_project)
        return;
    for (const auto &item : m_project->items()) {
        for (const auto &slice : item.slices) {
            if (slice.status != QStringLiteral("active"))
                continue;
            const QString path = sliceAudioPath(slice.id);
            if (path.isEmpty() || !QFile::exists(path))
                m_missingAudioIds.insert(slice.id);
        }
    }
}

void ProjectDataSource::rescanAudioAvailability() {
    m_audioScanned = false;
    QSet<QString> oldMissing = m_missingAudioIds;
    ensureAudioScanned();
    if (oldMissing != m_missingAudioIds)
        emit audioAvailabilityChanged();
}

PipelineContext *ProjectDataSource::context(const QString &sliceId) {
    QMutexLocker locker(&m_contextsMutex);
    
    auto it = m_contexts.find(sliceId);
    if (it != m_contexts.end())
        return &it->second;

    // Lazy-load from disk
    const QString path = contextPath(sliceId);
    if (QFile::exists(path)) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            auto data = file.readAll();
            auto j = nlohmann::json::parse(data.constData(), data.constData() + data.size(),
                                           nullptr, false);
            if (!j.is_discarded()) {
                auto ctxResult = PipelineContext::fromJson(j);
                if (ctxResult) {
                    m_contexts[sliceId] = std::move(ctxResult.value());
                    return &m_contexts[sliceId];
                }
            }
        }
    }

    // Create empty context
    PipelineContext ctx;
    ctx.itemId = sliceId;
    ctx.audioPath = sliceAudioPath(sliceId);
    m_contexts[sliceId] = std::move(ctx);
    return &m_contexts[sliceId];
}

Result<void> ProjectDataSource::saveContext(const QString &sliceId) {
    QMutexLocker locker(&m_contextsMutex);

    auto it = m_contexts.find(sliceId);
    if (it == m_contexts.end())
        return Result<void>::Error("No context loaded for slice: " + sliceId.toStdString());

    const QString path = contextPath(sliceId);

    auto j = it->second.toJson();
    auto str = j.dump(2);
    return dsfw::AtomicFileWriter::write(dsfw::PathUtils::toStdPath(path), str);
}

QString ProjectDataSource::contextPath(const QString &sliceId) const {
    return ProjectPaths::sliceContextPath(m_workingDir, sliceId);
}

QString ProjectDataSource::dstextPath(const QString &sliceId) const {
    return ProjectPaths::sliceDstextPath(m_workingDir, sliceId);
}

QString ProjectDataSource::sliceAudioPath(const QString &sliceId) const {
    if (m_project) {
        for (const auto &item : m_project->items()) {
            for (const auto &slice : item.slices) {
                if (slice.id == sliceId) {
                    QString audioPath = item.audioSource;
                    if (QDir::isRelativePath(audioPath))
                        audioPath = QDir(m_workingDir).absoluteFilePath(audioPath);
                    if (QFile::exists(audioPath))
                        return audioPath;

                    // V.7: Heuristic fallback — try wavs/<sliceId>.wav
                    QString fallback = QDir(m_workingDir).absoluteFilePath(
                        QStringLiteral("wavs/%1.wav").arg(sliceId));
                    if (QFile::exists(fallback)) {
                        qInfo() << "ProjectDataSource: audioSource not found:"
                                << audioPath << "— using fallback:" << fallback;
                        return fallback;
                    }

                    qWarning() << "ProjectDataSource: audio file not found for slice"
                               << sliceId << "at" << audioPath << "(fallback also missing)";
                    return audioPath;
                }
            }
        }
    }
    // Final fallback via ProjectPaths
    QString fallbackPath = ProjectPaths::sliceAudioPath(m_workingDir, sliceId);
    qWarning() << "ProjectDataSource: slice" << sliceId
               << "not found in project items, falling back to:" << fallbackPath;
    return fallbackPath;
}

const Slice *ProjectDataSource::findSlice(const QString &sliceId) const {
    if (!m_project)
        return nullptr;
    for (const auto &item : m_project->items()) {
        for (const auto &slice : item.slices) {
            if (slice.id == sliceId)
                return &slice;
        }
    }
    return nullptr;
}

} // namespace dstools
