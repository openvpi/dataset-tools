#pragma once

#include <dsfw/ITaskProcessor.h>

namespace examples {

class WordCountProcessor : public dstools::ITaskProcessor {
public:
    QString processorId() const override;
    QString displayName() const override;
    dstools::TaskSpec taskSpec() const override;
    dstools::ProcessorConfig capabilities() const override;

    dstools::Result<void> initialize(dstools::IModelManager &mm,
                                     const dstools::ProcessorConfig &config) override;
    void release() override;

    dstools::Result<dstools::TaskOutput> process(const dstools::TaskInput &input) override;
};

} // namespace examples
