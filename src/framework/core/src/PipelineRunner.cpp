#include <dsfw/PipelineRunner.h>
#include <dsfw/TaskProcessorRegistry.h>
#include <dsfw/FormatAdapterRegistry.h>

namespace dstools {

PipelineRunner::PipelineRunner(QObject *parent) : QObject(parent) {}

Result<void> PipelineRunner::run(const PipelineOptions &opts,
                                  std::vector<PipelineContext> &contexts) {
    for (int stepIdx = 0; stepIdx < (int)opts.steps.size(); ++stepIdx) {
        const auto &step = opts.steps[stepIdx];

        // Create processor
        auto processor = TaskProcessorRegistry::instance().create(step.taskName, step.processorId);
        if (!processor && !step.optional)
            return Result<void>::Error("Processor not found: " + step.processorId.toStdString());
        if (!processor)
            continue; // optional step, skip

        const auto spec = processor->taskSpec();

        // Process each context
        for (int itemIdx = 0; itemIdx < (int)contexts.size(); ++itemIdx) {
            auto &ctx = contexts[itemIdx];

            // Skip discarded/error items
            if (ctx.status != PipelineContext::Status::Active)
                continue;

            // Manual step: skip (UI handles this externally)
            if (step.manual)
                continue;

            emit progress(stepIdx, itemIdx, (int)contexts.size(), step.taskName);

            // Optional import
            if (!step.importFormat.isEmpty()) {
                auto *adapter = FormatAdapterRegistry::instance().adapter(step.importFormat);
                if (adapter) {
                    auto importPath = step.importPath.isEmpty() ? ctx.audioPath : step.importPath;
                    auto res = adapter->importToLayers(importPath, ctx.layers, step.config);
                    if (!res.ok() && !step.optional) {
                        ctx.status = PipelineContext::Status::Error;
                        ctx.discardReason = QString::fromStdString(res.error());
                        continue;
                    }
                }
            }

            // Merge step config into context globalConfig
            ProcessorConfig mergedConfig = ctx.globalConfig;
            for (auto it = step.config.begin(); it != step.config.end(); ++it)
                mergedConfig[it.key()] = it.value();
            ctx.globalConfig = mergedConfig;

            // Build input
            auto inputRes = ctx.buildTaskInput(spec);
            if (!inputRes.ok()) {
                if (step.optional)
                    continue;
                ctx.status = PipelineContext::Status::Error;
                ctx.discardReason = QString::fromStdString(inputRes.error());
                continue;
            }

            // Process
            auto outputRes = processor->process(inputRes.value());
            if (!outputRes.ok()) {
                ctx.status = PipelineContext::Status::Error;
                ctx.discardReason = QString::fromStdString(outputRes.error());

                StepRecord rec;
                rec.stepName = step.taskName;
                rec.processorId = step.processorId;
                rec.success = false;
                rec.errorMessage = QString::fromStdString(outputRes.error());
                rec.usedConfig = step.config;
                ctx.stepHistory.push_back(rec);
                continue;
            }

            // Apply output
            ctx.applyTaskOutput(spec, outputRes.value());
            ctx.completedSteps.append(step.taskName);

            // Record step
            StepRecord rec;
            rec.stepName = step.taskName;
            rec.processorId = step.processorId;
            rec.success = true;
            rec.usedConfig = step.config;
            ctx.stepHistory.push_back(rec);

            // Validation
            if (step.validator) {
                QString reason;
                auto validStatus = step.validator(ctx, spec, reason);
                if (validStatus == PipelineContext::Status::Discarded) {
                    ctx.status = PipelineContext::Status::Discarded;
                    ctx.discardReason = reason;
                    ctx.discardedAtStep = step.taskName;
                    emit itemDiscarded(ctx.itemId, reason);
                }
            }

            // Optional export
            if (!step.exportFormat.isEmpty()) {
                auto *adapter = FormatAdapterRegistry::instance().adapter(step.exportFormat);
                if (adapter) {
                    auto exportPath = step.exportPath.isEmpty() ? ctx.audioPath : step.exportPath;
                    adapter->exportFromLayers(ctx.layers, exportPath, step.config);
                }
            }
        }

        emit stepCompleted(stepIdx, step.taskName);
    }

    return Result<void>::Ok();
}

} // namespace dstools
