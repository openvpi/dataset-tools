#pragma once

#include <dsfw/ITaskProcessor.h>

namespace dstools {

class PassthroughProcessor : public ITaskProcessor {
public:
    QString processorId() const override;
    QString displayName() const override;
    TaskSpec taskSpec() const override;
    Result<void> initialize(IModelManager &mm, const ProcessorConfig &modelConfig) override;
    void release() override;
    Result<TaskOutput> process(const TaskInput &input) override;
};

} // namespace dstools
