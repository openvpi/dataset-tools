#include <dstools/ProjectPaths.h>

namespace dstools {

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
