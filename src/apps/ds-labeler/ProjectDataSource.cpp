#include "ProjectDataSource.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>

#include <nlohmann/json.hpp>

namespace dstools {

ProjectDataSource::ProjectDataSource(QObject *parent) : IEditorDataSource(parent) {}

void ProjectDataSource::setProject(DsProject *project, const QString &workingDir) {
    m_project = project;
    m_workingDir = workingDir;
    m_contexts.clear();
    emit sliceListChanged();
}

void ProjectDataSource::clear() {
    m_project = nullptr;
    m_workingDir.clear();
    m_contexts.clear();
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
    return m_workingDir + QStringLiteral("/dstemp/contexts/") + sliceId +
           QStringLiteral(".json");
}

QString ProjectDataSource::dstextPath(const QString &sliceId) const {
    return m_workingDir + QStringLiteral("/dstemp/dstext/") + sliceId +
           QStringLiteral(".dstext");
}

QString ProjectDataSource::sliceAudioPath(const QString &sliceId) const {
    return m_workingDir + QStringLiteral("/dstemp/slices/") + sliceId +
           QStringLiteral(".wav");
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
