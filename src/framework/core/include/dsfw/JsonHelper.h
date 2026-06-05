#pragma once

#include <dstools/Result.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

// ADL overloads for QString <-> nlohmann::json
#ifdef QT_CORE_LIB
#    include <QString>
inline void to_json(nlohmann::json &j, const QString &s) {
    j = s.toStdString();
}
inline void from_json(const nlohmann::json &j, QString &s) {
    s = QString::fromStdString(j.get<std::string>());
}
#endif

namespace dstools {

    class JsonHelper {
    public:
        [[nodiscard]] static Result<nlohmann::json> loadFile(const std::filesystem::path &path);
        [[nodiscard]] static Result<void> saveFile(const std::filesystem::path &path, const nlohmann::json &data, int indent = 4);

        template <typename T>
        static T get(const nlohmann::json &j, const std::string &key, const T &defaultValue) {
            if (j.contains(key) && !j[key].is_null()) {
                try {
                    return j[key].get<T>();
                } catch (const nlohmann::json::type_error &e) {
                    return defaultValue;
                } catch (const nlohmann::json::out_of_range &e) {
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

        template <typename K, typename V>
        static Result<std::map<K, V>> getRequiredMap(const nlohmann::json &root, const char *path) {
            const auto *node = resolve(root, path);
            if (!node) {
                return Err<std::map<K, V>>(std::string("missing required field: ") + path);
            }
            if (!node->is_object()) {
                return Err<std::map<K, V>>(std::string("expected object at ") + path);
            }
            try {
                return Ok(node->get<std::map<K, V>>());
            } catch (const std::exception &e) {
                return Err<std::map<K, V>>(std::string("error reading object at ") + path + ": " + e.what());
            }
        }

        template <typename T>
        static Result<std::vector<T>> getRequiredVec(const nlohmann::json &root, const char *path) {
            const auto *node = resolve(root, path);
            if (!node) {
                return Err<std::vector<T>>(std::string("missing required field: ") + path);
            }
            if (!node->is_array()) {
                return Err<std::vector<T>>(std::string("expected array at ") + path);
            }
            try {
                return Ok(node->get<std::vector<T>>());
            } catch (const std::exception &e) {
                return Err<std::vector<T>>(std::string("error reading array at ") + path + ": " + e.what());
            }
        }
    };

} // namespace dstools
