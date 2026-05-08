#include <dstools/DsTextTypes.h>

#include <nlohmann/json.hpp>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <algorithm>

namespace dstools {

void IntervalLayer::sortBoundaries() {
    std::stable_sort(boundaries.begin(), boundaries.end(),
                     [](const Boundary &a, const Boundary &b) { return a.pos < b.pos; });
}

int IntervalLayer::nextId() const {
    if (boundaries.empty())
        return 1;
    int maxId = 0;
    for (const auto &b : boundaries)
        maxId = std::max(maxId, b.id);
    return maxId + 1;
}

Result<DsTextDocument> DsTextDocument::load(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return Result<DsTextDocument>::Error("Cannot open file: " + path.toStdString());

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(file.readAll().toStdString());
    } catch (const std::exception &e) {
        return Result<DsTextDocument>::Error(std::string("JSON parse error: ") + e.what());
    }

    auto version = QString::fromStdString(j.value("version", "2.0.0"));

    DsTextDocument doc;
    doc.version = version;

    if (j.contains("audio")) {
        const auto &a = j["audio"];
        doc.audio.path = QString::fromStdString(a.value("path", ""));
        doc.audio.in = a.value("in", int64_t(0));
        doc.audio.out = a.value("out", int64_t(0));
    }

    if (j.contains("layers")) {
        for (const auto &jl : j["layers"]) {
            IntervalLayer layer;
            layer.name = QString::fromStdString(jl.value("name", ""));
            layer.type = QString::fromStdString(jl.value("type", ""));
            if (jl.contains("boundaries")) {
                for (const auto &jb : jl["boundaries"]) {
                    Boundary b;
                    b.id = jb.value("id", 0);
                    b.pos = jb.value("pos", int64_t(0));
                    b.text = QString::fromStdString(jb.value("text", ""));
                    layer.boundaries.push_back(b);
                }
            }
            layer.sortBoundaries();
            doc.layers.push_back(std::move(layer));
        }
    }

    if (j.contains("curves")) {
        for (const auto &jc : j["curves"]) {
            CurveLayer curve;
            curve.name = QString::fromStdString(jc.value("name", ""));
            curve.type = QString::fromStdString(jc.value("type", "curve"));
            curve.timestep = jc.value("timestep", int64_t(0));
            if (jc.contains("values")) {
                curve.values = jc["values"].get<std::vector<int32_t>>();
            }
            doc.curves.push_back(std::move(curve));
        }
    }

    if (j.contains("groups")) {
        for (const auto &jg : j["groups"]) {
            doc.groups.push_back(jg.get<std::vector<int>>());
        }
    }

    if (j.contains("dependencies")) {
        for (const auto &jd : j["dependencies"]) {
            LayerDependency dep;
            dep.parentLayerIndex = jd.value("parent", -1);
            dep.childLayerIndex = jd.value("child", -1);
            if (jd.contains("parentName") && jd["parentName"].is_string())
                dep.parentLayerName = QString::fromStdString(jd["parentName"].get<std::string>());
            if (jd.contains("childName") && jd["childName"].is_string())
                dep.childLayerName = QString::fromStdString(jd["childName"].get<std::string>());
            dep.parentStartBoundaryId = jd.value("parentStartBoundaryId", -1);
            dep.parentEndBoundaryId = jd.value("parentEndBoundaryId", -1);
            dep.childStartBoundaryId = jd.value("childStartBoundaryId", -1);
            dep.childEndBoundaryId = jd.value("childEndBoundaryId", -1);
            if (jd.contains("childBoundaryIds") && jd["childBoundaryIds"].is_array()) {
                for (const auto &id : jd["childBoundaryIds"])
                    dep.childBoundaryIds.push_back(id.get<int>());
            }
            doc.dependencies.push_back(dep);
        }
    }

    if (j.contains("meta") && j["meta"].is_object()) {
        const auto &m = j["meta"];
        if (m.contains("editedSteps") && m["editedSteps"].is_array()) {
            for (const auto &s : m["editedSteps"])
                if (s.is_string())
                    doc.meta.editedSteps.append(QString::fromStdString(s.get<std::string>()));
        }
    }

    return Result<DsTextDocument>::Ok(std::move(doc));
}

Result<void> DsTextDocument::save(const QString &path) const {
    nlohmann::json j;
    j["version"] = version.toStdString();

    j["audio"] = {
        {"path", audio.path.toStdString()},
        {"in",   audio.in},
        {"out",  audio.out}
    };

    j["layers"] = nlohmann::json::array();
    for (const auto &layer : layers) {
        nlohmann::json jl;
        jl["name"] = layer.name.toStdString();
        if (!layer.type.isEmpty())
            jl["type"] = layer.type.toStdString();
        jl["boundaries"] = nlohmann::json::array();
        for (const auto &b : layer.boundaries) {
            jl["boundaries"].push_back({
                {"id",   b.id},
                {"pos",  b.pos},
                {"text", b.text.toStdString()}
            });
        }
        j["layers"].push_back(jl);
    }

    j["curves"] = nlohmann::json::array();
    for (const auto &curve : curves) {
        nlohmann::json jc;
        jc["name"] = curve.name.toStdString();
        jc["type"] = curve.type.toStdString();
        jc["timestep"] = curve.timestep;
        jc["values"] = curve.values;
        j["curves"].push_back(jc);
    }

    j["groups"] = nlohmann::json::array();
    for (const auto &g : groups) {
        j["groups"].push_back(g);
    }

    j["dependencies"] = nlohmann::json::array();
    for (const auto &dep : dependencies) {
        auto jd = nlohmann::json::object();
        jd["parent"] = dep.parentLayerIndex;
        jd["child"] = dep.childLayerIndex;
        if (!dep.parentLayerName.isEmpty())
            jd["parentName"] = dep.parentLayerName.toStdString();
        if (!dep.childLayerName.isEmpty())
            jd["childName"] = dep.childLayerName.toStdString();
        if (dep.parentStartBoundaryId >= 0)
            jd["parentStartBoundaryId"] = dep.parentStartBoundaryId;
        if (dep.parentEndBoundaryId >= 0)
            jd["parentEndBoundaryId"] = dep.parentEndBoundaryId;
        if (dep.childStartBoundaryId >= 0)
            jd["childStartBoundaryId"] = dep.childStartBoundaryId;
        if (dep.childEndBoundaryId >= 0)
            jd["childEndBoundaryId"] = dep.childEndBoundaryId;
        if (!dep.childBoundaryIds.empty())
            jd["childBoundaryIds"] = dep.childBoundaryIds;
        j["dependencies"].push_back(jd);
    }

    if (!meta.editedSteps.isEmpty()) {
        nlohmann::json editedArr = nlohmann::json::array();
        for (const auto &s : meta.editedSteps)
            editedArr.push_back(s.toStdString());
        j["meta"]["editedSteps"] = editedArr;
    }

    // Ensure parent directory exists
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return Result<void>::Error("Cannot write file: " + path.toStdString());

    auto str = j.dump(4);
    file.write(str.data(), static_cast<qint64>(str.size()));
    return Result<void>::Ok();
}

const BindingGroup *DsTextDocument::findGroup(int boundaryId) const {
    for (const auto &g : groups) {
        for (int id : g) {
            if (id == boundaryId)
                return &g;
        }
    }
    return nullptr;
}

std::vector<int> DsTextDocument::boundIdsOf(int boundaryId) const {
    const auto *g = findGroup(boundaryId);
    if (!g)
        return {};
    std::vector<int> result;
    for (int id : *g) {
        if (id != boundaryId)
            result.push_back(id);
    }
    return result;
}

} // namespace dstools
