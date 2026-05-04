#include <dstools/AudioFileResolver.h>
#include <dstools/ProjectPaths.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace dstools {

// ─── AudioFileResolver ────────────────────────────────────────────────────────

QStringList AudioFileResolver::audioExtensions() {
    return {"wav", "mp3", "m4a", "flac", "ogg"};
}

QString AudioFileResolver::audioToDataFile(const QString &audioPath, const QString &suffix) {
    QFileInfo info(audioPath);
    QString audioSuffix = info.suffix().toLower();
    QString fileName = info.fileName();
    QString baseName = fileName.mid(0, fileName.size() - audioSuffix.size() - 1);
    // Non-wav audio: encode original suffix into filename
    if (audioSuffix != "wav")
        baseName += "_" + audioSuffix;
    return info.absolutePath() + "/" + baseName + "." + suffix;
}

QString AudioFileResolver::findAudioFile(const QString &dataFilePath) {
    QFileInfo fi(dataFilePath);
    QString dir = fi.absolutePath();
    QString baseName = fi.completeBaseName(); // e.g. "song" or "song_mp3"

    auto exts = audioExtensions();

    // 1. Simple same-name: try baseName.wav, baseName.mp3, ...
    for (const QString &ext : exts) {
        QString testPath = dir + "/" + baseName + "." + ext;
        if (QFile::exists(testPath))
            return testPath;
    }

    // 2. Suffix-encoded: "song_mp3" → try "song.mp3"
    for (const QString &ext : exts) {
        QString encodedSuffix = "_" + ext;
        if (baseName.endsWith(encodedSuffix, Qt::CaseInsensitive)) {
            QString realBase = baseName.mid(0, baseName.size() - encodedSuffix.size());
            QString testPath = dir + "/" + realBase + "." + ext;
            if (QFile::exists(testPath))
                return testPath;
        }
    }

    return {};
}

// ─── ProjectPaths ─────────────────────────────────────────────────────────────

QString ProjectPaths::dsItemsDir(const QString &workingDir) {
    return workingDir + QStringLiteral("/dstemp/dsitems");
}

QString ProjectPaths::slicesDir(const QString &workingDir) {
    return workingDir + QStringLiteral("/dstemp/slices");
}

QString ProjectPaths::contextsDir(const QString &workingDir) {
    return workingDir + QStringLiteral("/dstemp/contexts");
}

QString ProjectPaths::dstextDir(const QString &workingDir) {
    return workingDir + QStringLiteral("/dstemp/dstext");
}

QString ProjectPaths::alignmentReviewDir(const QString &workingDir) {
    return workingDir + QStringLiteral("/dstemp/alignment_review");
}

QString ProjectPaths::buildCsvDir(const QString &workingDir) {
    return workingDir + QStringLiteral("/dstemp/build_csv");
}

QString ProjectPaths::transcriptionsCsvPath(const QString &workingDir) {
    return workingDir + QStringLiteral("/dstemp/build_csv/transcriptions.csv");
}

QString ProjectPaths::buildDsDir(const QString &workingDir) {
    return workingDir + QStringLiteral("/dstemp/build_ds");
}

QString ProjectPaths::slicerOutputDir(const QString &workingDir) {
    return workingDir + QStringLiteral("/dstemp/slicer_output");
}

QString ProjectPaths::wavsDir(const QString &outputDir) {
    return outputDir + QStringLiteral("/wavs");
}

QString ProjectPaths::dsDir(const QString &outputDir) {
    return outputDir + QStringLiteral("/ds");
}

QString ProjectPaths::sliceAudioPath(const QString &workingDir, const QString &sliceId) {
    return slicesDir(workingDir) + QStringLiteral("/") + sliceId + QStringLiteral(".wav");
}

QString ProjectPaths::sliceContextPath(const QString &workingDir, const QString &sliceId) {
    return contextsDir(workingDir) + QStringLiteral("/") + sliceId + QStringLiteral(".json");
}

QString ProjectPaths::sliceDstextPath(const QString &workingDir, const QString &sliceId) {
    return dstextDir(workingDir) + QStringLiteral("/") + sliceId + QStringLiteral(".dstext");
}

QString ProjectPaths::slicerOutputAudioPath(const QString &workingDir, const QString &name) {
    return slicerOutputDir(workingDir) + QStringLiteral("/") + name + QStringLiteral(".wav");
}

} // namespace dstools
