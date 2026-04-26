#pragma once

#include <QString>
#include <dsfw/TaskTypes.h>
#include <dstools/Result.h>
#include <vector>

namespace dstools {

    struct PipelineContext;

    void registerDomainFormatAdapters();

    Result<void> exportContextsToCsv(const std::vector<PipelineContext> &contexts, const QString &outputPath,
                                     const ProcessorConfig &config = {});

} // namespace dstools
