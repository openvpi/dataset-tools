#pragma once

#include <dstools/Result.h>

#include <filesystem>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

// ADL overloads for QString <-> nlohmann::json
#ifdef QT_CORE_LIB
#include <QString>
inline void to_json(nlohmann::json &j, const QString &s) { j = s.toStdString(); }
inline void from_json(const nlohmann::json &j, QString &s) { s = QString::fromStdString(j.get<std::string>()); }
#endif

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

    /// @brief Resolve a slash-delimited path in a JSON object.
    /// @return Pointer to the node, or nullptr if not found.
    static const nlohmann::json *resolve(const nlohmann::json &root, const char *path) {
        if (!path || path[0] == '\0')
            return nullptr;
        const nlohmann::json *node = &root;
        std::string segment;
        std::istringstream ss(path);
        while (std::getline(ss, segment, '/')) {
            if (segment.empty())
                continue;
            if (!node->is_object() || !node->contains(segment))
                return nullptr;
            node = &(*node)[segment];
        }
        return node;
    }

    static nlohmann::json getObject(const nlohmann::json &j, const std::string &key) {
        if (j.contains(key) && j[key].is_object()) {
            return j[key];
        }
        return nlohmann::json::object();
    }
};

} // namespace dstools
