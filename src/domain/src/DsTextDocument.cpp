#include <dstools/DsTextTypes.h>

#include <nlohmann/json.hpp>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <algorithm>
#include <cmath>

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

    // v1 → v2 migration
    if (version.startsWith("1.")) {
        if (j.contains("layers")) {
            for (auto &layer : j["layers"]) {
                if (layer.contains("boundaries")) {
                    for (auto &bd : layer["boundaries"]) {
                        if (bd.contains("position")) {
                            double pos = bd["position"].get<double>();
                            bd["pos"] = static_cast<int64_t>(std::llround(pos * 1e6));
                            bd.erase("position");
                        }
                    }
                }
            }
        }
        version = "2.0.0";
        j["version"] = "2.0.0";
    }

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
        j["curves"].push_back({
            {"name",     curve.name.toStdString()},
            {"timestep", curve.timestep},
            {"values",   curve.values}
        });
    }

    j["groups"] = nlohmann::json::array();
    for (const auto &g : groups) {
        j["groups"].push_back(g);
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
