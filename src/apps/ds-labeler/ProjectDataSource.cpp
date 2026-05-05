#include "ProjectDataSource.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QDebug>

#include <nlohmann/json.hpp>

#include <dstools/ProjectPaths.h>

namespace dstools {

ProjectDataSource::ProjectDataSource(QObject *parent) : IEditorDataSource(parent) {}

void ProjectDataSource::setProject(DsProject *project, const QString &workingDir) {
    m_project = project;
    m_workingDir = workingDir;
    m_contexts.clear();
    m_audioScanned = false;
    m_missingAudioIds.clear();
    emit sliceListChanged();
}

void ProjectDataSource::clear() {
    m_project = nullptr;
    m_workingDir.clear();
    m_contexts.clear();
    m_audioScanned = false;
    m_missingAudioIds.clear();
    emit sliceListChanged();
}

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
    auto it = m_contexts.find(sliceId);
    if (it != m_contexts.end())
        return it->second.dirty;
    return {};
}

void ProjectDataSource::clearDirtyLayers(const QString &sliceId,
                                          const QStringList &layers) {
    auto it = m_contexts.find(sliceId);
    if (it == m_contexts.end())
        return;
    for (const auto &layer : layers)
        it->second.dirty.removeAll(layer);
    saveContext(sliceId);
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
    auto it = m_contexts.find(sliceId);
    if (it == m_contexts.end())
        return Result<void>::Error("No context loaded for slice: " + sliceId.toStdString());

    const QString path = contextPath(sliceId);
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return Result<void>::Error("Cannot write context: " + path.toStdString());

    auto j = it->second.toJson();
    auto str = j.dump(2);
    file.write(str.c_str(), static_cast<qint64>(str.size()));
    return Result<void>::Ok();
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
                        qWarning() << "ProjectDataSource: audioSource not found:"
                                   << audioPath << "— using fallback:" << fallback;
                        return fallback;
                    }

                    // Return original path even if missing (caller uses validatedAudioPath)
                    qWarning() << "ProjectDataSource: audio file not found for slice"
                               << sliceId << "at" << audioPath;
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
