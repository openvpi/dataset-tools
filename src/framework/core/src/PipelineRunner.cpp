#include <dsfw/PipelineRunner.h>
#include <dsfw/AtomicFileWriter.h>
#include <dsfw/JsonHelper.h>
#include <dsfw/Log.h>
#include <dsfw/PathUtils.h>
#include <dsfw/TaskProcessorRegistry.h>
#include <dsfw/FormatAdapterRegistry.h>

#include <QDir>
#include <QFileInfo>
#include <atomic>
#include <memory>
#include <nlohmann/json.hpp>

namespace dsfw {

PipelineRunner::PipelineRunner(QObject* parent) : QObject(parent) {
}

Result<void> PipelineRunner::run(const PipelineOptions& opts, std::vector<PipelineContext>& contexts) {
    return run(opts, contexts, nullptr);
}

Result<void> PipelineRunner::run(const PipelineOptions& opts, std::vector<PipelineContext>& contexts,
                                 std::shared_ptr<std::atomic<bool>> cancelToken) {
    const int totalSteps = static_cast<int>(opts.steps.size());

    saveSnapshot(opts, contexts, 0, totalSteps);

    for (int stepIdx = 0; stepIdx < totalSteps; ++stepIdx) {
        if (cancelToken && !*cancelToken) {
            emit cancelled();
            return Result<void>::Error("Pipeline cancelled");
        }
        const auto& step = opts.steps[stepIdx];

        // Create processor
        auto processor = TaskProcessorRegistry::instance().create(step.taskName, step.processorId);
        if (!processor && !step.optional) {
            DSFW_LOG_ERROR("pipeline", (std::string("Processor not found: ") + step.processorId.toStdString()).c_str());
            return Result<void>::Error("Processor not found: " + step.processorId.toStdString());
        }
        if (!processor)
            continue; // optional step, skip

        const auto spec = processor->taskSpec();

        // Process each context
        for (int itemIdx = 0; itemIdx < static_cast<int>(contexts.size()); ++itemIdx) {
            if (cancelToken && !*cancelToken) {
                emit cancelled();
                return Result<void>::Error("Pipeline cancelled");
            }
            auto& ctx = contexts[itemIdx];

            // Skip discarded/error items
            if (ctx.status != PipelineContext::Status::Active)
                continue;

            // Manual step: skip (UI handles this externally)
            if (step.manual)
                continue;

            emit progress(stepIdx, itemIdx, static_cast<int>(contexts.size()), step.taskName);

            // Optional import
            if (!step.importFormat.isEmpty()) {
                auto* adapter = FormatAdapterRegistry::instance().adapter(step.importFormat);
                if (adapter) {
                    auto importPath = step.importPath.isEmpty() ? ctx.audioPath : step.importPath;
                    auto tempLayers = ctx.layers;
                    auto res = adapter->importToLayers(importPath, tempLayers, step.config);
                    if (!res.ok()) {
                        if (!step.optional) {
                            ctx.status = PipelineContext::Status::Error;
                            ctx.discardReason = QString::fromStdString(res.error());
                            DSFW_LOG_ERROR("pipeline",
                                           (ctx.itemId.toStdString() + " import failed: " + res.error()).c_str());
                            continue;
                        }
                    } else {
                        ctx.layers = std::move(tempLayers);
                    }
                }
            }

            // Merge step config into context globalConfig
            ProcessorConfig mergedConfig = ctx.globalConfig;
            for (auto it = step.config.begin(); it != step.config.end(); ++it)
                mergedConfig[it->first] = it->second;
            ctx.globalConfig = mergedConfig;

            auto precondRes = ctx.checkPreconditions(spec);
            if (!precondRes.ok()) {
                if (step.optional)
                    continue;
                ctx.status = PipelineContext::Status::Error;
                ctx.discardReason = QString::fromStdString(precondRes.error());
                DSFW_LOG_ERROR("pipeline",
                               (ctx.itemId.toStdString() + " precondition failed: " + precondRes.error()).c_str());
                continue;
            }

            // Build input
            auto inputRes = ctx.buildTaskInput(spec);
            if (!inputRes.ok()) {
                if (step.optional)
                    continue;
                ctx.status = PipelineContext::Status::Error;
                ctx.discardReason = QString::fromStdString(inputRes.error());
                DSFW_LOG_ERROR("pipeline",
                               (ctx.itemId.toStdString() + " buildInput failed: " + inputRes.error()).c_str());
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

                DSFW_LOG_ERROR("pipeline", (ctx.itemId.toStdString() + " " + step.taskName.toStdString() +
                                            " failed: " + outputRes.error())
                                               .c_str());
                continue;
            }

            // Apply output
            ctx.applyTaskOutput(spec, outputRes.value());

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
                auto* adapter = FormatAdapterRegistry::instance().adapter(step.exportFormat);
                if (adapter) {
                    auto exportPath = step.exportPath.isEmpty() ? ctx.audioPath : step.exportPath;
                    auto exportRes = adapter->exportFromLayers(ctx.layers, exportPath, step.config);
                    if (!exportRes.ok() && !step.optional) {
                        ctx.status = PipelineContext::Status::Error;
                        ctx.discardReason = QString::fromStdString(exportRes.error());
                        DSFW_LOG_ERROR("pipeline",
                                       (ctx.itemId.toStdString() + " export failed: " + exportRes.error()).c_str());
                    }
                }
            }
        }

        emit stepCompleted(stepIdx, step.taskName);
        saveSnapshot(opts, contexts, stepIdx + 1, totalSteps);
    }

    return Result<void>::Ok();
}

void PipelineRunner::saveSnapshot(const PipelineOptions& opts, const std::vector<PipelineContext>& contexts,
                                  int currentStep, int totalSteps) {
    if (opts.snapshotDir.isEmpty())
        return;

    QDir dir(opts.snapshotDir);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    nlohmann::json snapshot;
    snapshot["version"] = 1;
    snapshot["currentStep"] = currentStep;
    snapshot["totalSteps"] = totalSteps;
    snapshot["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();

    nlohmann::json ctxArray = nlohmann::json::array();
    for (const auto& ctx : contexts) {
        ctxArray.push_back(nlohmann::json::parse(ctx.toJsonString()));
    }
    snapshot["contexts"] = std::move(ctxArray);

    const std::string content = snapshot.dump(4);
    const auto stepStr = QStringLiteral("step_%1").arg(currentStep, 3, 10, QLatin1Char('0'));
    const auto filename = dsfw::PathUtils::toStdPath(opts.snapshotDir) / (stepStr.toStdWString() + L".json");

    dsfw::AtomicFileWriter::writeJson(filename, content);

    cleanupOldSnapshots(opts.snapshotDir);
}

Result<std::pair<std::vector<PipelineContext>, int>> PipelineRunner::loadLatestSnapshot(const QString& snapshotDir) {
    QDir dir(snapshotDir);
    if (!dir.exists())
        return Result<std::pair<std::vector<PipelineContext>, int>>::Error("Snapshot directory not found");

    const auto filters = QStringList{QStringLiteral("step_*.json")};
    const auto entries = dir.entryInfoList(filters, QDir::Files, QDir::Name | QDir::Reversed);
    if (entries.isEmpty())
        return Result<std::pair<std::vector<PipelineContext>, int>>::Error("No snapshots found");

    const auto& latest = entries.first();
    auto loadResult = dstools::JsonHelper::loadFile(dsfw::PathUtils::toStdPath(latest.absoluteFilePath()));
    if (!loadResult.ok())
        return Result<std::pair<std::vector<PipelineContext>, int>>::Error("Failed to read snapshot: " +
                                                                           loadResult.error());

    const auto& j = loadResult.value();

    std::vector<PipelineContext> contexts;
    if (j.contains("contexts") && j.at("contexts").is_array()) {
        for (const auto& ctxJson : j.at("contexts")) {
            auto result = PipelineContext::fromJson(ctxJson);
            if (result.ok())
                contexts.push_back(std::move(result.value()));
        }
    }

    int currentStep = j.value("currentStep", 0);
    return std::make_pair(std::move(contexts), currentStep);
}

void PipelineRunner::cleanupOldSnapshots(const QString& snapshotDir) {
    QDir dir(snapshotDir);
    if (!dir.exists())
        return;

    const auto filters = QStringList{QStringLiteral("step_*.json")};
    auto entries = dir.entryInfoList(filters, QDir::Files, QDir::Name | QDir::Reversed);

    while (entries.size() > kMaxSnapshots) {
        const auto& oldest = entries.takeLast();
        QFile::remove(oldest.absoluteFilePath());
    }
}

bool PipelineRunner::hasLatestSnapshot(const QString& snapshotDir) {
    QDir dir(snapshotDir);
    if (!dir.exists())
        return false;

    const auto filters = QStringList{QStringLiteral("step_*.json")};
    const auto entries = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    return !entries.isEmpty();
}

} // namespace dsfw
