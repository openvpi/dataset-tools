#include <dstools/DomainInit.h>
#include <dsfw/FormatAdapterRegistry.h>
#include <dsfw/PipelineContext.h>
#include "CsvAdapter.h"
#include "DsFileAdapter.h"
#include "TextGridAdapter.h"

namespace dstools {

void registerDomainFormatAdapters() {
    auto &registry = FormatAdapterRegistry::instance();
    registry.registerAdapter(std::make_unique<TextGridAdapter>());
    registry.registerAdapter(std::make_unique<CsvAdapter>());
    registry.registerAdapter(std::make_unique<DsFileAdapter>());
}

Result<void> exportContextsToCsv(const std::vector<PipelineContext> &contexts,
                                  const QString &outputPath,
                                  const ProcessorConfig &config) {
    return CsvAdapter::batchExport(contexts, outputPath, config);
}

} // namespace dstools
