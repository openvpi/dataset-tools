#pragma once

/// @file JsonHelper.h
/// @brief Safe wrapper around nlohmann::json for file I/O and value access.
///
/// All methods handle errors gracefully -- no unhandled exceptions, no crashes.
/// Parse/type errors are reported via std::string& error output parameters
/// or by returning fallback defaults.
///
/// Usage:
/// @code
///   using namespace dstools;
///   std::string err;
///
///   // Load from file (returns null json on failure)
///   auto config = JsonHelper::loadFile("config.json", err);
///   if (!err.empty()) { /* handle error */ }
///
///   // Safe value access with defaults
///   int sr = JsonHelper::get(config, "mel_spec_config/sample_rate", 44100);
///   auto langs = JsonHelper::getMap<std::string, int>(config, "languages");
///
///   // Save to file atomically
///   if (!JsonHelper::saveFile("out.json", config, err)) { /* handle error */ }
/// @endcode

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <map>
#include <vector>

#include <QString>

namespace dstools {

class JsonHelper {
public:
    // ── File I/O ──────────────────────────────────────────────────────

    /// Load and parse a JSON file. Returns empty object on failure.
    /// @param path     File path (std::filesystem::path or std::string)
    /// @param error    Set to error message on failure, empty on success
    /// @return Parsed JSON, or empty object `{}` if load/parse fails
    static nlohmann::json loadFile(const std::filesystem::path &path, std::string &error);

    /// Save JSON to file atomically (write temp + rename).
    /// @param path     Output file path
    /// @param data     JSON to write
    /// @param error    Set to error message on failure
    /// @param indent   Pretty-print indent (default 4, use -1 for compact)
    /// @return true on success
    static bool saveFile(const std::filesystem::path &path, const nlohmann::json &data,
                         std::string &error, int indent = 4);

    // ── Safe value access ─────────────────────────────────────────────

    /// Get a value by "/"-separated path with a typed default.
    /// Returns defaultValue if the path doesn't exist or the type doesn't match.
    /// Never throws.
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

    /// Get a value by simple key (no nesting) with a typed default.
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

    // Specialization for QString
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

    /// Get a nested object by "/"-separated path. Returns empty object if not found.
    static nlohmann::json getObject(const nlohmann::json &root, const char *path);

    /// Get a nested array by "/"-separated path. Returns empty array if not found.
    static nlohmann::json getArray(const nlohmann::json &root, const char *path);

    /// Check if a "/"-separated path exists and is a specific type.
    static bool contains(const nlohmann::json &root, const char *path);

    // ── Iteration helpers ─────────────────────────────────────────────

    /// Get a std::map from a JSON object. Returns empty map on type mismatch.
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

    /// Get a std::vector from a JSON array. Returns empty vector on type mismatch.
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

    // ── Required value access (reports errors instead of silent defaults) ──

    /// Get a required value by "/"-separated path. Sets error if missing or wrong type.
    /// @return true on success, false on failure (value is left unchanged)
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

    /// Get a required std::vector by "/"-separated path. Sets error if missing or wrong type.
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

    /// Get a required std::map by "/"-separated path. Sets error if missing or wrong type.
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

    // ── Path utilities ────────────────────────────────────────────────

    /// Navigate into nested JSON by "/"-separated path.
    /// Returns nullptr if any segment is missing or not an object.
    static const nlohmann::json *resolve(const nlohmann::json &root, const char *path);

private:
    JsonHelper() = default; // Static-only class
};

} // namespace dstools
