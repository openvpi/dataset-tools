#pragma once

#include <QString>

namespace dstools {

class ProjectPaths {
public:
    static QString dsItemsDir(const QString &workingDir);
    static QString slicesDir(const QString &workingDir);
    static QString contextsDir(const QString &workingDir);
    static QString dstextDir(const QString &workingDir);
    static QString alignmentReviewDir(const QString &workingDir);
    static QString buildCsvDir(const QString &workingDir);
    static QString transcriptionsCsvPath(const QString &workingDir);
    static QString buildDsDir(const QString &workingDir);
    static QString slicerOutputDir(const QString &workingDir);
    static QString wavsDir(const QString &outputDir);
    static QString dsDir(const QString &outputDir);

    static QString sliceAudioPath(const QString &workingDir, const QString &sliceId);
    static QString sliceContextPath(const QString &workingDir, const QString &sliceId);
    static QString sliceDstextPath(const QString &workingDir, const QString &sliceId);
    static QString slicerOutputAudioPath(const QString &workingDir, const QString &name);
};

} // namespace dstools
