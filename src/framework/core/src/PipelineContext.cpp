#include <dsfw/PipelineContext.h>

namespace dstools {

Result<TaskInput> PipelineContext::buildTaskInput(const TaskSpec &spec) const {
    TaskInput input;
    input.audioPath = audioPath;
    input.config = globalConfig;

    for (const auto &slot : spec.inputs) {
        auto it = layers.find(slot.category);
        if (it == layers.end())
            return Result<TaskInput>::Error(
                "Missing layer for category: " + slot.category.toStdString());
        input.layers[slot.name] = it->second;
    }
    return Result<TaskInput>::Ok(std::move(input));
}

void PipelineContext::applyTaskOutput(const TaskSpec &spec, const TaskOutput &output) {
    for (const auto &slot : spec.outputs) {
        auto it = output.layers.find(slot.name);
        if (it != output.layers.end())
            layers[slot.category] = it->second;
    }
}

static QString statusToString(PipelineContext::Status s) {
    switch (s) {
        case PipelineContext::Status::Active:    return QStringLiteral("active");
        case PipelineContext::Status::Discarded: return QStringLiteral("discarded");
        case PipelineContext::Status::Error:     return QStringLiteral("error");
    }
    return QStringLiteral("active");
}

static Result<PipelineContext::Status> statusFromString(const QString &s) {
    if (s == QStringLiteral("active"))    return PipelineContext::Status::Active;
    if (s == QStringLiteral("discarded")) return PipelineContext::Status::Discarded;
    if (s == QStringLiteral("error"))     return PipelineContext::Status::Error;
    return Result<PipelineContext::Status>::Error("Unknown status: " + s.toStdString());
}

nlohmann::json PipelineContext::toJson() const {
    nlohmann::json j;
    j["audioPath"] = audioPath.toStdString();
    j["itemId"] = itemId.toStdString();
    j["status"] = statusToString(status).toStdString();
    j["discardReason"] = discardReason.toStdString();
    j["discardedAtStep"] = discardedAtStep.toStdString();
    j["globalConfig"] = globalConfig;

    // Layers
    nlohmann::json jLayers = nlohmann::json::object();
    for (const auto &[key, val] : layers)
        jLayers[key.toStdString()] = val;
    j["layers"] = jLayers;

    // Step history
    nlohmann::json jHistory = nlohmann::json::array();
    for (const auto &rec : stepHistory) {
        nlohmann::json r;
        r["stepName"] = rec.stepName.toStdString();
        r["processorId"] = rec.processorId.toStdString();
        r["startTime"] = rec.startTime.toString(Qt::ISODate).toStdString();
        r["endTime"] = rec.endTime.toString(Qt::ISODate).toStdString();
        r["success"] = rec.success;
        r["errorMessage"] = rec.errorMessage.toStdString();
        r["usedConfig"] = rec.usedConfig;
        jHistory.push_back(r);
    }
    j["stepHistory"] = jHistory;

    // Edited steps
    if (!editedSteps.isEmpty()) {
        nlohmann::json jEdited = nlohmann::json::array();
        for (const auto &s : editedSteps)
            jEdited.push_back(s.toStdString());
        j["editedSteps"] = jEdited;
    }

    // Dirty layers
    if (!dirty.isEmpty()) {
        nlohmann::json jDirty = nlohmann::json::array();
        for (const auto &d : dirty)
            jDirty.push_back(d.toStdString());
        j["dirty"] = jDirty;
    }

    return j;
}

Result<PipelineContext> PipelineContext::fromJson(const nlohmann::json &j) {
    try {
    if (!j.contains("audioPath") || !j.contains("itemId"))
        return Result<PipelineContext>::Error("Missing required fields");

    PipelineContext ctx;
    ctx.audioPath = QString::fromStdString(j.at("audioPath").get<std::string>());
    ctx.itemId = QString::fromStdString(j.at("itemId").get<std::string>());

    if (j.contains("status")) {
        auto sr = statusFromString(QString::fromStdString(j.at("status").get<std::string>()));
        if (!sr.ok())
            return Result<PipelineContext>::Error(sr.error());
        ctx.status = sr.value();
    }

    if (j.contains("discardReason"))
        ctx.discardReason = QString::fromStdString(j.at("discardReason").get<std::string>());
    if (j.contains("discardedAtStep"))
        ctx.discardedAtStep = QString::fromStdString(j.at("discardedAtStep").get<std::string>());
    if (j.contains("globalConfig"))
        ctx.globalConfig = j.at("globalConfig");

    if (j.contains("layers")) {
        for (auto &[key, val] : j.at("layers").items())
            ctx.layers[QString::fromStdString(key)] = val;
    }

    // Legacy completedSteps → ignore (derived from stepHistory now)
    // Kept for backward compatibility during loading

    if (j.contains("stepHistory")) {
        for (const auto &r : j.at("stepHistory")) {
            StepRecord rec;
            rec.stepName = QString::fromStdString(r.at("stepName").get<std::string>());
            rec.processorId = QString::fromStdString(r.at("processorId").get<std::string>());
            rec.startTime = QDateTime::fromString(
                QString::fromStdString(r.at("startTime").get<std::string>()), Qt::ISODate);
            rec.endTime = QDateTime::fromString(
                QString::fromStdString(r.at("endTime").get<std::string>()), Qt::ISODate);
            rec.success = r.at("success").get<bool>();
            rec.errorMessage = QString::fromStdString(r.at("errorMessage").get<std::string>());
            rec.usedConfig = r.at("usedConfig");
            ctx.stepHistory.push_back(rec);
        }
    }

    if (j.contains("editedSteps")) {
        for (const auto &s : j.at("editedSteps"))
            ctx.editedSteps.append(QString::fromStdString(s.get<std::string>()));
    }

    if (j.contains("dirty")) {
        for (const auto &d : j.at("dirty"))
            ctx.dirty.append(QString::fromStdString(d.get<std::string>()));
    }

    return Result<PipelineContext>::Ok(std::move(ctx));
    } catch (const std::exception &e) {
        return Result<PipelineContext>::Error(std::string("PipelineContext::fromJson failed: ") + e.what());
    }
}

QStringList PipelineContext::completedSteps() const {
    QStringList result;
    for (const auto &rec : stepHistory) {
        if (rec.success)
            result.append(rec.stepName);
    }
    return result;
}

} // namespace dstools
