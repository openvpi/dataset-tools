#include <dstools/ExportFormats.h>
#include <dsfw/JsonHelper.h>

#include <fstream>
#include <filesystem>

namespace dstools {

    Result<void> HtsLabelExportFormat::exportItem(const std::filesystem::path &sourceFile,
                                                   const std::filesystem::path &workingDir,
                                                   const std::filesystem::path &outputPath) {
        auto jsonResult = JsonHelper::loadFile(sourceFile);
        if (!jsonResult)
            return Err(jsonResult.error());

        const auto &j = jsonResult.value();
        std::ofstream out(outputPath);
        if (!out.is_open()) {
            return Err("Cannot open output file: " + outputPath.string());
        }

        (void)workingDir;

        try {
            if (j.contains("phonemes") && j["phonemes"].is_array()) {
                for (const auto &ph : j["phonemes"]) {
                    out << ph.value("start", 0) << " "
                        << ph.value("end", 0) << " "
                        << ph.value("symbol", "") << "\n";
                }
            }
        } catch (const std::exception &e) {
            return Err(std::string("Failed to write HTS label: ") + e.what());
        }
        return Ok();
    }

    Result<void> HtsLabelExportFormat::exportAll(const std::filesystem::path &workingDir,
                                                   const std::filesystem::path &outputDir) {
        try {
            for (const auto &entry : std::filesystem::directory_iterator(workingDir)) {
                if (entry.path().extension() == ".dsitem") {
                    auto dst = outputDir / entry.path().stem();
                    dst += ".lab";
                    auto result = exportItem(entry.path(), workingDir, dst);
                    if (!result)
                        return result;
                }
            }
        } catch (const std::exception &e) {
            return Err(std::string("Failed to scan working directory: ") + e.what());
        }
        return Ok();
    }

    Result<void> SinsyXmlExportFormat::exportItem(const std::filesystem::path &sourceFile,
                                                    const std::filesystem::path &workingDir,
                                                    const std::filesystem::path &outputPath) {
        auto jsonResult = JsonHelper::loadFile(sourceFile);
        if (!jsonResult)
            return Err(jsonResult.error());

        const auto &j = jsonResult.value();
        std::ofstream out(outputPath);
        if (!out.is_open()) {
            return Err("Cannot open output file: " + outputPath.string());
        }

        (void)workingDir;

        try {
            out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            out << "<sinsy>\n";
            if (j.contains("phonemes") && j["phonemes"].is_array()) {
                for (const auto &ph : j["phonemes"]) {
                    out << "  <ph start=\"" << ph.value("start", 0)
                        << "\" end=\"" << ph.value("end", 0)
                        << "\" symbol=\"" << ph.value("symbol", "") << "\"/>\n";
                }
            }
            out << "</sinsy>\n";
        } catch (const std::exception &e) {
            return Err(std::string("Failed to write Sinsy XML: ") + e.what());
        }
        return Ok();
    }

    Result<void> SinsyXmlExportFormat::exportAll(const std::filesystem::path &workingDir,
                                                    const std::filesystem::path &outputDir) {
        try {
            for (const auto &entry : std::filesystem::directory_iterator(workingDir)) {
                if (entry.path().extension() == ".dsitem") {
                    auto dst = outputDir / entry.path().stem();
                    dst += ".xml";
                    auto result = exportItem(entry.path(), workingDir, dst);
                    if (!result)
                        return result;
                }
            }
        } catch (const std::exception &e) {
            return Err(std::string("Failed to scan working directory: ") + e.what());
        }
        return Ok();
    }

} // namespace dstools
