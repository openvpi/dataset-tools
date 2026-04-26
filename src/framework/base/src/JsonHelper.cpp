#include <dsfw/JsonHelper.h>
#include <dsfw/PathUtils.h>

#include <fstream>

namespace dstools {

    Result<nlohmann::json> JsonHelper::loadFile(const std::filesystem::path &path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Err<nlohmann::json>("Cannot open file: " + dsfw::PathUtils::toUtf8(path));
        }
        try {
            nlohmann::json data = nlohmann::json::parse(file);
            return Ok(std::move(data));
        } catch (const nlohmann::json::parse_error &e) {
            return Err<nlohmann::json>(std::string("JSON parse error: ") + e.what());
        } catch (const nlohmann::json::type_error &e) {
            return Err<nlohmann::json>(std::string("JSON type error: ") + e.what());
        }
    }

    Result<void> JsonHelper::saveFile(const std::filesystem::path &path, const nlohmann::json &data, const int indent) {
        auto tmpPath = path;
        tmpPath += ".tmp";
        {
            std::ofstream file(tmpPath);
            if (!file.is_open()) {
                return Err("Cannot open file for writing: " + dsfw::PathUtils::toUtf8(path));
            }
            file << data.dump(indent);
            if (!file.good()) {
                file.close();
                std::filesystem::remove(tmpPath);
                return Err("Failed to write JSON file: " + dsfw::PathUtils::toUtf8(path));
            }
        }
        std::error_code ec;
        std::filesystem::rename(tmpPath, path, ec);
        if (ec) {
            std::filesystem::remove(tmpPath);
            return Err("Failed to finalize JSON file: " + dsfw::PathUtils::toUtf8(path));
        }
        return Ok();
    }

} // namespace dstools
