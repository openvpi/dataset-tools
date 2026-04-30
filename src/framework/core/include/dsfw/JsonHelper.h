#pragma once

/// @file JsonHelper.h
/// @brief Safe wrapper around nlohmann::json for file I/O and value access.
///
/// All methods handle errors gracefully -- no unhandled exceptions, no crashes.
/// Parse/type errors are reported via std::string& error output parameters
/// or by returning fallback defaults.

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <map>
#include <vector>

#include <QString>

namespace dstools {

class JsonHelper {
public:
    static nlohmann::json loadFile(const std::filesystem::path &path, std::string &error);
    static bool saveFile(const std::filesystem::path &path, const nlohmann::json &data,
                         std::string &error, int indent = 4);

    template <typename T>
    static T get(const nlohmann::json &root, const char *path, const T &defaultValue) {
        try {
            const auto *node = resolve(root, path);
            if (!node) return defaultValue;
            return node->get<T>();
        } catch (const std::exception &) {
            return defaultValue;
        }
    }

    template <typename T>
    static T get(const nlohmann::json &root, const std::string &key, const T &defaultValue) {
        try {
            if (!root.is_object() || !root.contains(key))
                return defaultValue;
            return root[key].get<T>();
        } catch (const std::exception &) {
            return defaultValue;
        }
    }

    template <>
    static QString get(const nlohmann::json &root, const char *path, const QString &defaultValue) {
        try {
            const auto *node = resolve(root, path);
            if (!node) return defaultValue;
            if (node->is_string()) {
                return QString::fromStdString(node->get<std::string>());
            }
            return defaultValue;
        } catch (const std::exception &) {
            return defaultValue;
        }
    }

    template <>
    static QString get(const nlohmann::json &root, const std::string &key, const QString &defaultValue) {
        try {
            if (!root.is_object() || !root.contains(key))
                return defaultValue;
            const auto &node = root[key];
            if (node.is_string()) {
                return QString::fromStdString(node.get<std::string>());
            }
            return defaultValue;
        } catch (const std::exception &) {
            return defaultValue;
        }
    }

    static nlohmann::json getObject(const nlohmann::json &root, const char *path);
    static nlohmann::json getArray(const nlohmann::json &root, const char *path);
    static bool contains(const nlohmann::json &root, const char *path);

    template <typename K, typename V>
    static std::map<K, V> getMap(const nlohmann::json &root, const char *path) {
        try {
            const auto *node = resolve(root, path);
            if (!node || !node->is_object()) return {};
            return node->get<std::map<K, V>>();
        } catch (const std::exception &) {
            return {};
        }
    }

    template <typename T>
    static std::vector<T> getVec(const nlohmann::json &root, const char *path) {
        try {
            const auto *node = resolve(root, path);
            if (!node || !node->is_array()) return {};
            return node->get<std::vector<T>>();
        } catch (const std::exception &) {
            return {};
        }
    }

    template <typename T>
    static bool getRequired(const nlohmann::json &root, const char *path, T &out, std::string &error) {
        try {
            const auto *node = resolve(root, path);
            if (!node) {
                error = std::string("missing required field: ") + path;
                return false;
            }
            out = node->get<T>();
            return true;
        } catch (const nlohmann::json::type_error &e) {
            error = std::string("type mismatch at ") + path + ": " + e.what();
            return false;
        } catch (const std::exception &e) {
            error = std::string("error reading ") + path + ": " + e.what();
            return false;
        }
    }

    template <typename T>
    static bool getRequiredVec(const nlohmann::json &root, const char *path, std::vector<T> &out,
                               std::string &error) {
        const auto *node = resolve(root, path);
        if (!node) {
            error = std::string("missing required field: ") + path;
            return false;
        }
        if (!node->is_array()) {
            error = std::string("expected array at ") + path;
            return false;
        }
        try {
            out = node->get<std::vector<T>>();
            return true;
        } catch (const std::exception &e) {
            error = std::string("error reading array at ") + path + ": " + e.what();
            return false;
        }
    }

    template <typename K, typename V>
    static bool getRequiredMap(const nlohmann::json &root, const char *path, std::map<K, V> &out,
                               std::string &error) {
        const auto *node = resolve(root, path);
        if (!node) {
            error = std::string("missing required field: ") + path;
            return false;
        }
        if (!node->is_object()) {
            error = std::string("expected object at ") + path;
            return false;
        }
        try {
            out = node->get<std::map<K, V>>();
            return true;
        } catch (const std::exception &e) {
            error = std::string("error reading object at ") + path + ": " + e.what();
            return false;
        }
    }

    static const nlohmann::json *resolve(const nlohmann::json &root, const char *path);

private:
    JsonHelper() = default;
};

} // namespace dstools
