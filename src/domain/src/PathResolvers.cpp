#include <dstools/AudioFileResolver.h>
#include <dstools/ProjectPaths.h>
#include <dsfw/PathUtils.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace dstools {

QStringList AudioFileResolver::audioExtensions() {
    return {"wav", "mp3", "m4a", "flac", "ogg"};
}

QString AudioFileResolver::audioToDataFile(const QString &audioPath, const QString &suffix) {
    QFileInfo info(audioPath);
    QString audioSuffix = info.suffix().toLower();
    QString fileName = info.fileName();
    QString baseName = fileName.mid(0, fileName.size() - audioSuffix.size() - 1);
    if (audioSuffix != "wav")
        baseName += "_" + audioSuffix;
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(info.absolutePath()),
                               (baseName + "." + suffix).toStdString()));
}

QString AudioFileResolver::findAudioFile(const QString &dataFilePath) {
    QFileInfo fi(dataFilePath);
    QString dir = fi.absolutePath();
    QString baseName = fi.completeBaseName();

    auto exts = audioExtensions();

    for (const QString &ext : exts) {
        QString testPath = dsfw::PathUtils::fromStdPath(
            dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(dir),
                                   (baseName + "." + ext).toStdString()));
        if (QFile::exists(testPath))
            return testPath;
    }

    for (const QString &ext : exts) {
        QString encodedSuffix = "_" + ext;
        if (baseName.endsWith(encodedSuffix, Qt::CaseInsensitive)) {
            QString realBase = baseName.mid(0, baseName.size() - encodedSuffix.size());
            QString testPath = dsfw::PathUtils::fromStdPath(
                dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(dir),
                                       (realBase + "." + ext).toStdString()));
            if (QFile::exists(testPath))
                return testPath;
        }
    }

    return {};
}

QString ProjectPaths::dsItemsDir(const QString &workingDir) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(workingDir), "dstemp/dsitems"));
}

QString ProjectPaths::slicesDir(const QString &workingDir) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(workingDir), "dstemp/slices"));
}

QString ProjectPaths::contextsDir(const QString &workingDir) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(workingDir), "dstemp/contexts"));
}

QString ProjectPaths::dstextDir(const QString &workingDir) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(workingDir), "dstemp/dstext"));
}

QString ProjectPaths::alignmentReviewDir(const QString &workingDir) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(workingDir), "dstemp/alignment_review"));
}

QString ProjectPaths::buildCsvDir(const QString &workingDir) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(workingDir), "dstemp/build_csv"));
}

QString ProjectPaths::transcriptionsCsvPath(const QString &workingDir) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(workingDir), "dstemp/build_csv/transcriptions.csv"));
}

QString ProjectPaths::buildDsDir(const QString &workingDir) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(workingDir), "dstemp/build_ds"));
}

QString ProjectPaths::slicerOutputDir(const QString &workingDir) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(workingDir), "dstemp/slicer_output"));
}

QString ProjectPaths::wavsDir(const QString &outputDir) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(outputDir), "wavs"));
}

QString ProjectPaths::dsDir(const QString &outputDir) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(outputDir), "ds"));
}

QString ProjectPaths::sliceAudioPath(const QString &workingDir, const QString &sliceId) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(slicesDir(workingDir)),
                               (sliceId + ".wav").toStdString()));
}

QString ProjectPaths::sliceContextPath(const QString &workingDir, const QString &sliceId) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(contextsDir(workingDir)),
                               (sliceId + ".json").toStdString()));
}

QString ProjectPaths::sliceDstextPath(const QString &workingDir, const QString &sliceId) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(dstextDir(workingDir)),
                               (sliceId + ".dstext").toStdString()));
}

QString ProjectPaths::slicerOutputAudioPath(const QString &workingDir, const QString &name) {
    return dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(slicerOutputDir(workingDir)),
                               (name + ".wav").toStdString()));
}

} // namespace dstools
