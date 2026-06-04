#include <dsfw/PipelineContext.h>
#include <dsfw/ConfigTypesJson.h>
#include <dsfw/TaskTypes.h>

#include <QSet>
#include <queue>

namespace dstools {

// Layer dependency DAG: modified layer → downstream layers that become dirty.
// Based on pipeline.md §3.1 I/O contract:
//   grapheme → phoneme → ph_num
//   midi reads audio directly (no layer dependency in basic path)
//   pitch reads audio directly (no layer dependency)
//   sentence does NOT auto-propagate to grapheme (ADR-53: grapheme is manual)
const std::map<QString, QStringList> PipelineContext::layerDag = {
    {QStringLiteral("grapheme"), {QStringLiteral("phoneme"), QStringLiteral("ph_num")}},
    {QStringLiteral("phoneme"),  {QStringLiteral("ph_num")}},
};

void PipelineContext::propagateDirty(const QString &modifiedLayer) {
    QSet<QString> dirtySet(dirty.begin(), dirty.end());

    std::queue<QString> queue;
    queue.push(modifiedLayer);

    while (!queue.empty()) {
        QString current = queue.front();
        queue.pop();

        auto it = layerDag.find(current);
        if (it == layerDag.end())
            continue;

        for (const auto &downstream : it->second) {
            if (!dirtySet.contains(downstream)) {
                dirtySet.insert(downstream);
                queue.push(downstream);
            }
        }
    }

    dirty.clear();
    for (const auto &d : dirtySet)
        dirty.append(d);
}

void PipelineContext::addDirtyLayer(const QString &layer) {
    if (!dirty.contains(layer))
        dirty.append(layer);
}

void PipelineContext::removeDirtyLayer(const QString &layer) {
    dirty.removeAll(layer);
}

void PipelineContext::setLayerManuallyEdited(const QString &layer, bool edited) {
    if (edited) {
        if (!manuallyEdited.contains(layer))
            manuallyEdited.append(layer);
    } else {
        manuallyEdited.removeAll(layer);
    }
}

bool PipelineContext::isLayerManuallyEdited(const QString &layer) const {
    return manuallyEdited.contains(layer);
}

void PipelineContext::addEditedStep(const QString &step) {
    if (!editedSteps.contains(step))
        editedSteps.append(step);
}

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

Result<void> PipelineContext::checkPreconditions(const TaskSpec &spec) const {
    if (status != Status::Active)
        return Result<void>::Error("Context is not active");

    for (const auto &slot : spec.inputs) {
        auto it = layers.find(slot.category);
        if (it == layers.end())
            return Result<void>::Error(
                "Missing required layer '" + slot.category.toStdString() +
                "' for step '" + spec.taskName.toStdString() + "'");
        if (it->second.empty())
            return Result<void>::Error(
                "Required layer '" + slot.category.toStdString() +
                "' is empty for step '" + spec.taskName.toStdString() + "'");
    }

    return Result<void>::Ok();
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
    j["globalConfig"] = configMapToJson(globalConfig);

    // Layers
    nlohmann::json jLayers = nlohmann::json::object();
    for (const auto &[key, val] : layers)
        jLayers[key.toStdString()] = val.toJson();
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
        r["usedConfig"] = configMapToJson(rec.usedConfig);
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

    // Manually edited layers
    if (!manuallyEdited.isEmpty()) {
        nlohmann::json jEdited = nlohmann::json::array();
        for (const auto &s : manuallyEdited)
            jEdited.push_back(s.toStdString());
        j["manuallyEdited"] = jEdited;
    }

    return j;
}

Result<void> PipelineContext::validateFromString(const std::string &jsonStr) {
    try {
        auto j = nlohmann::json::parse(jsonStr);
        return validate(j);
    } catch (const nlohmann::json::exception &e) {
        return Result<void>::Error(std::string("PipelineContext: JSON parse error: ") + e.what());
    }
}

Result<void> PipelineContext::validate(const nlohmann::json &j) {
    if (!j.is_object())
        return Result<void>::Error("PipelineContext: not a JSON object");

    if (!j.contains("audioPath") || !j["audioPath"].is_string())
        return Result<void>::Error("PipelineContext: missing or invalid 'audioPath'");
    if (!j.contains("itemId") || !j["itemId"].is_string())
        return Result<void>::Error("PipelineContext: missing or invalid 'itemId'");

    if (j.contains("status") && j["status"].is_string()) {
        QString s = QString::fromStdString(j["status"].get<std::string>());
        if (s != QStringLiteral("active") && s != QStringLiteral("discarded") && s != QStringLiteral("error"))
            return Result<void>::Error("PipelineContext: invalid status value: " + s.toStdString());
    }

    if (j.contains("layers") && j["layers"].is_object()) {
        for (const auto &[key, val] : j["layers"].items()) {
            if (key.empty())
                return Result<void>::Error("PipelineContext: empty layer name in layers");
            if (!val.is_object())
                return Result<void>::Error("PipelineContext: layer '" + key + "' is not an object");
        }
    }

    if (j.contains("stepHistory") && j["stepHistory"].is_array()) {
        for (const auto &r : j["stepHistory"]) {
            if (!r.is_object())
                return Result<void>::Error("PipelineContext: stepHistory entry is not an object");
            if (!r.contains("stepName") || !r["stepName"].is_string())
                return Result<void>::Error("PipelineContext: stepHistory entry missing 'stepName'");
            if (!r.contains("processorId") || !r["processorId"].is_string())
                return Result<void>::Error("PipelineContext: stepHistory entry missing 'processorId'");
        }
    }

    return Result<void>::Ok();
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
        ctx.globalConfig = configMapFromJson(j.at("globalConfig"));

    if (j.contains("layers")) {
        for (auto &[key, val] : j.at("layers").items())
            ctx.layers[QString::fromStdString(key)] = LayerData::fromJson(val);
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
            rec.usedConfig = configMapFromJson(r.at("usedConfig"));
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

    if (j.contains("manuallyEdited")) {
        for (const auto &s : j.at("manuallyEdited"))
            ctx.manuallyEdited.append(QString::fromStdString(s.get<std::string>()));
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

QStringList PipelineContext::deriveDirtySteps(const std::vector<TaskSpec> &taskSpecs) const {
    QStringList dirtySteps;
    
    // Collect all layers produced by each step
    std::map<QString, QStringList> stepOutputLayers;
    for (const auto &spec : taskSpecs) {
        QStringList outputs;
        for (const auto &slot : spec.outputs) {
            outputs.append(slot.category);
        }
        stepOutputLayers[spec.taskName] = outputs;
    }
    
    // For each dirty layer, find steps that produce it
    for (const QString &dirtyLayer : dirty) {
        for (const auto &[stepName, outputLayers] : stepOutputLayers) {
            if (outputLayers.contains(dirtyLayer)) {
                // Check if this step depends on any dirty layer as input
                bool dependsOnDirty = false;
                for (const auto &spec : taskSpecs) {
                    if (spec.taskName == stepName) {
                        for (const auto &inputSlot : spec.inputs) {
                            if (dirty.contains(inputSlot.category)) {
                                dependsOnDirty = true;
                                break;
                            }
                        }
                        break;
                    }
                }
                
                // If step depends on dirty input, it needs to be re-executed
                if (dependsOnDirty && !dirtySteps.contains(stepName)) {
                    dirtySteps.append(stepName);
                }
            }
        }
    }
    
    return dirtySteps;
}

std::string PipelineContext::toJsonString() const {
    return toJson().dump();
}

Result<PipelineContext> PipelineContext::fromJsonString(const std::string &jsonStr) {
    auto j = nlohmann::json::parse(jsonStr, nullptr, false);
    if (j.is_discarded())
        return Result<PipelineContext>::Error("PipelineContext::fromJsonString: invalid JSON");
    auto validationResult = validate(j);
    if (!validationResult)
        return Result<PipelineContext>::Error(validationResult.error());
    return fromJson(j);
}

} // namespace dstools
