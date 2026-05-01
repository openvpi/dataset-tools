#pragma once

#include <dstools/Result.h>

#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

namespace dstools {

class JsonHelper {
public:
    static Result<nlohmann::json> loadFile(const std::filesystem::path &path);
    static Result<void> saveFile(const std::filesystem::path &path, const nlohmann::json &data, int indent = 4);

    template <typename T>
    static T get(const nlohmann::json &j, const std::string &key, const T &defaultValue) {
        if (j.contains(key) && !j[key].is_null()) {
            try {
                return j[key].get<T>();
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    static nlohmann::json getObject(const nlohmann::json &j, const std::string &key) {
        if (j.contains(key) && j[key].is_object()) {
            return j[key];
        }
        return nlohmann::json::object();
    }
};

} // namespace dstools
