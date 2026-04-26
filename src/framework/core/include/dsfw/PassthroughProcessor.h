#pragma once

#include <dsfw/ITaskProcessor.h>

namespace dstools {

    class PassthroughProcessor : public ITaskProcessor {
    public:
        QString processorId() const override;
        QString displayName() const override;
        TaskSpec taskSpec() const override;
        [[nodiscard]] Result<void> initialize(ModelManager &mm, const ProcessorConfig &modelConfig) override;
        void release() override;
        [[nodiscard]] Result<TaskOutput> process(const TaskInput &input) override;
    };

} // namespace dstools
