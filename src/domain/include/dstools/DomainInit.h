#pragma once

#include <QString>
#include <dsfw/PipelineContext.h>
#include <dsfw/TaskTypes.h>
#include <dsfw/Result.h>
#include <vector>

namespace dstools {

    void registerDomainFormatAdapters();

    Result<void> exportContextsToCsv(const std::vector<PipelineContext> &contexts, const QString &outputPath,
                                     const ProcessorConfig &config = {});

} // namespace dstools
