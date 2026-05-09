#include <dsfw/JsonHelper.h>
#include <dstools/PathEncoding.h>

#include <fstream>

namespace dstools {

    Result<nlohmann::json> JsonHelper::loadFile(const std::filesystem::path &path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Err<nlohmann::json>("Cannot open file: " + dstools::pathToUtf8(path));
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
        std::ofstream file(path);
        if (!file.is_open()) {
            return Err("Cannot open file for writing: " + dstools::pathToUtf8(path));
        }
        file << data.dump(indent);
        if (!file.good()) {
            return Err("Failed to write JSON file: " + dstools::pathToUtf8(path));
        }
        return Ok();
    }

} // namespace dstools
