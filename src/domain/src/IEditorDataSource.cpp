/// @file IEditorDataSource.cpp
/// @brief Default implementations for IEditorDataSource audio validity methods.

#include <dstools/IEditorDataSource.h>

#include <QFile>

namespace dstools {

bool IEditorDataSource::audioExists(const QString &sliceId) const {
    const QString path = audioPath(sliceId);
    return !path.isEmpty() && QFile::exists(path);
}

QString IEditorDataSource::validatedAudioPath(const QString &sliceId) const {
    const QString path = audioPath(sliceId);
    if (path.isEmpty() || !QFile::exists(path))
        return {};
    return path;
}

QStringList IEditorDataSource::missingAudioSlices() const {
    QStringList missing;
    for (const auto &id : sliceIds()) {
        if (!audioExists(id))
            missing.append(id);
    }
    return missing;
}

} // namespace dstools
