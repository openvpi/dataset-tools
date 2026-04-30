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

/// @brief Safe wrapper around nlohmann::json for file I/O and value access.
///
/// All methods handle errors gracefully -- no unhandled exceptions, no crashes.
/// Parse/type errors are reported via std::string& error output parameters
/// or by returning fallback defaults.
///
/// @code
/// std::string err;
/// auto json = JsonHelper::loadFile("config.json", err);
/// int val = JsonHelper::get(json, "audio/sampleRate", 44100);
/// @endcode
class JsonHelper {
public:
    /// @brief Load and parse a JSON file.
    /// @param path Filesystem path to the JSON file.
    /// @param error Populated with error description on failure.
    /// @return Parsed JSON object, or null on failure.
    static nlohmann::json loadFile(const std::filesystem::path &path, std::string &error);
    /// @brief Save a JSON object to a file.
    /// @param path Destination filesystem path.
    /// @param data JSON data to write.
    /// @param error Populated with error description on failure.
    /// @param indent Pretty-print indentation width (default 4).
    /// @return True on success.
    static bool saveFile(const std::filesystem::path &path, const nlohmann::json &data,
                         std::string &error, int indent = 4);

    /// @brief Get a value by slash-delimited path, returning defaultValue if absent or wrong type.
    /// @tparam T Target type.
    /// @param root JSON root object.
    /// @param path Slash-delimited path (e.g. "audio/sampleRate").
    /// @param defaultValue Fallback value.
    /// @return Resolved value or default.
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

    /// @brief Get a value by direct key lookup, returning defaultValue if absent or wrong type.
    /// @tparam T Target type.
    /// @param root JSON root object.
    /// @param key Direct key name.
    /// @param defaultValue Fallback value.
    /// @return Resolved value or default.
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

    /// @brief Get a JSON object at the given path, or null object if absent.
    /// @param root JSON root object.
    /// @param path Slash-delimited path.
    /// @return JSON object or null.
    static nlohmann::json getObject(const nlohmann::json &root, const char *path);
    /// @brief Get a JSON array at the given path, or empty array if absent.
    /// @param root JSON root object.
    /// @param path Slash-delimited path.
    /// @return JSON array or empty.
    static nlohmann::json getArray(const nlohmann::json &root, const char *path);
    /// @brief Check whether a path exists in the JSON tree.
    /// @param root JSON root object.
    /// @param path Slash-delimited path.
    /// @return True if the path resolves to a value.
    static bool contains(const nlohmann::json &root, const char *path);

    /// @brief Get a std::map from a JSON object at the given path.
    /// @tparam K Map key type.
    /// @tparam V Map value type.
    /// @param root JSON root object.
    /// @param path Slash-delimited path.
    /// @return Parsed map or empty map on failure.
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

    /// @brief Get a std::vector from a JSON array at the given path.
    /// @tparam T Element type.
    /// @param root JSON root object.
    /// @param path Slash-delimited path.
    /// @return Parsed vector or empty vector on failure.
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

    /// @brief Get a required value; returns false and sets error if missing or wrong type.
    /// @tparam T Target type.
    /// @param root JSON root object.
    /// @param path Slash-delimited path.
    /// @param out Output value, set on success.
    /// @param error Populated with error description on failure.
    /// @return True on success.
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

    /// @brief Get a required vector; returns false and sets error if missing or not an array.
    /// @tparam T Element type.
    /// @param root JSON root object.
    /// @param path Slash-delimited path.
    /// @param out Output vector, set on success.
    /// @param error Populated with error description on failure.
    /// @return True on success.
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

    /// @brief Get a required map; returns false and sets error if missing or not an object.
    /// @tparam K Map key type.
    /// @tparam V Map value type.
    /// @param root JSON root object.
    /// @param path Slash-delimited path.
    /// @param out Output map, set on success.
    /// @param error Populated with error description on failure.
    /// @return True on success.
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

    /// @brief Resolve a slash-delimited path to a JSON node pointer.
    /// @param root JSON root object.
    /// @param path Slash-delimited path.
    /// @return Pointer to the resolved node, or nullptr if not found.
    static const nlohmann::json *resolve(const nlohmann::json &root, const char *path);

private:
    JsonHelper() = default;
};

} // namespace dstools
