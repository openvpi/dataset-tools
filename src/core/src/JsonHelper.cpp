#include <dstools/JsonHelper.h>

#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <cstdio>   // _wrename, std::remove
#include <cstdlib>  // _wmktemp_s (optional)
#else
#include <cstdio>
#endif

namespace dstools {

nlohmann::json JsonHelper::loadFile(const std::filesystem::path &path, std::string &error) {
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

bool JsonHelper::saveFile(const std::filesystem::path &path, const nlohmann::json &data,
                          std::string &error, int indent) {
    error.clear();

    // Ensure parent directory exists
    if (auto parent = path.parent_path(); !parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            error = "cannot create directory " + parent.string() + ": " + ec.message();
            return false;
        }
    }

    // Write to temp file, then rename for atomicity
    auto tempPath = path;
    tempPath += ".tmp";

    {
        std::ofstream stream(tempPath, std::ios::out | std::ios::trunc);
        if (!stream.is_open()) {
            error = "cannot open temp file for writing: " + tempPath.string();
            return false;
        }

        try {
            stream << data.dump(indent) << '\n';
        } catch (const std::exception &e) {
            error = std::string("JSON serialization error: ") + e.what();
            stream.close();
            std::filesystem::remove(tempPath);
            return false;
        }

        stream.close();
        if (stream.fail()) {
            error = "write failed: " + tempPath.string();
            std::filesystem::remove(tempPath);
            return false;
        }
    }

    // Atomic rename
    std::error_code ec;
    std::filesystem::rename(tempPath, path, ec);
    if (ec) {
        // Rename can fail cross-device; fall back to copy+remove
        std::filesystem::copy_file(tempPath, path,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(tempPath);
        if (ec) {
            error = "cannot write to " + path.string() + ": " + ec.message();
            return false;
        }
    }

    return true;
}

nlohmann::json JsonHelper::getObject(const nlohmann::json &root, const char *path) {
    const auto *node = resolve(root, path);
    if (!node || !node->is_object()) return nlohmann::json::object();
    return *node;
}

nlohmann::json JsonHelper::getArray(const nlohmann::json &root, const char *path) {
    const auto *node = resolve(root, path);
    if (!node || !node->is_array()) return nlohmann::json::array();
    return *node;
}

bool JsonHelper::contains(const nlohmann::json &root, const char *path) {
    return resolve(root, path) != nullptr;
}

const nlohmann::json *JsonHelper::resolve(const nlohmann::json &root, const char *path) {
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

} // namespace dstools
