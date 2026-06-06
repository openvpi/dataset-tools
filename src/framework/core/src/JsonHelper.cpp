#include <dsfw/AtomicFileWriter.h>
#include <dsfw/JsonHelper.h>
#include <dsfw/PathUtils.h>

namespace dsfw {

    Result<nlohmann::json> JsonHelper::loadFile(const std::filesystem::path &path) {
        auto file = dsfw::PathUtils::openIfstream(path);
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
        return dsfw::AtomicFileWriter::writeJson(path, data.dump(indent));
    }

} // namespace dstools
