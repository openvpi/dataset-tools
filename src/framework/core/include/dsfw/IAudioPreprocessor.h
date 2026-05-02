#pragma once

#include <dsfw/TaskTypes.h>
#include <dstools/Result.h>

#include <QString>

namespace dstools {

class IAudioPreprocessor {
public:
    virtual ~IAudioPreprocessor() = default;
    virtual QString preprocessorId() const = 0;
    virtual QString displayName() const = 0;
    virtual Result<void> process(const QString &inputPath,
                                 const QString &outputPath,
                                 const ProcessorConfig &config) = 0;
};

} // namespace dstools
