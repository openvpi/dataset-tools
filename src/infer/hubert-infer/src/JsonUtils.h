#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace hubert_infer {

class JsonUtils {
public:
    static const nlohmann::json *resolve(const nlohmann::json &root, const char *path) {
        if (!path || path[0] == '\0')
            return nullptr;
        const nlohmann::json *node = &root;
        std::string segment;
        std::istringstream ss(path);
        while (std::getline(ss, segment, '/')) {
            if (segment.empty()) continue;
            if (!node->is_object() || !node->contains(segment))
                return nullptr;
            node = &(*node)[segment];
        }
        return node;
    }

    static nlohmann::json loadFile(const std::filesystem::path &path, std::string &error) {
        error.clear();
        if (!std::filesystem::exists(path)) {
            error = "file does not exist: " + path.string();
            return nlohmann::json::object();
        }
        std::ifstream stream(path);
        if (!stream.is_open()) {
            error = "cannot open file: " + path.string();
            return nlohmann::json::object();
        }
        try {
            auto data = nlohmann::json::parse(stream);
            if (!data.is_object() && !data.is_array()) {
                error = "JSON root is not an object or array: " + path.string();
                return nlohmann::json::object();
            }
            return data;
        } catch (const nlohmann::json::parse_error &e) {
            error = std::string("JSON parse error in ") + path.string() + ": " + e.what();
            return nlohmann::json::object();
        } catch (const std::exception &e) {
            error = std::string("error reading ") + path.string() + ": " + e.what();
            return nlohmann::json::object();
        }
    }

    static nlohmann::json getObject(const nlohmann::json &root, const char *path) {
        const auto *node = resolve(root, path);
        if (!node || !node->is_object()) return nlohmann::json::object();
        return *node;
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

private:
    JsonUtils() = default;
};

} // namespace hubert_infer
