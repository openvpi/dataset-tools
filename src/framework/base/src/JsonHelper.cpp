#include <dsfw/JsonHelper.h>

#include <fstream>

namespace dstools {

    Result<nlohmann::json> JsonHelper::loadFile(const std::filesystem::path &path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            const auto u8 = path.u8string();
            return Err<nlohmann::json>("Cannot open file: " + std::string(u8.begin(), u8.end()));
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
                const auto u8 = path.u8string();
                return Err("Cannot open file for writing: " + std::string(u8.begin(), u8.end()));
            }
            file << data.dump(indent);
            if (!file.good()) {
                file.close();
                std::filesystem::remove(tmpPath);
                const auto u8 = path.u8string();
                return Err("Failed to write JSON file: " + std::string(u8.begin(), u8.end()));
            }
        }
        std::error_code ec;
        std::filesystem::rename(tmpPath, path, ec);
        if (ec) {
            std::filesystem::remove(tmpPath);
            const auto u8 = path.u8string();
            return Err("Failed to finalize JSON file: " + std::string(u8.begin(), u8.end()));
        }
        return Ok();
    }

} // namespace dstools
