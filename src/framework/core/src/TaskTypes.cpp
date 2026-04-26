#include <dsfw/TaskTypes.h>

#include <nlohmann/json.hpp>

namespace dstools {

LayerData LayerData::fromJson(const nlohmann::json &j) {
    return LayerData(j.dump());
}

nlohmann::json LayerData::toJson() const {
    if (json.empty())
        return nlohmann::json();
    return nlohmann::json::parse(json, nullptr, false);
}

} // namespace dstools