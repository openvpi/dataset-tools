#pragma once

#include <QString>
#include <dsfw/PipelineContext.h>
#include <dsfw/TaskTypes.h>
#include <dsfw/Result.h>
#include <vector>

namespace dstools {

void registerDomainFormatAdapters();

    dsfw::Result<void> exportContextsToCsv(const std::vector<dsfw::PipelineContext> &contexts, const QString &outputPath,
                                     const dsfw::ProcessorConfig &config = {});

} // namespace dstools
