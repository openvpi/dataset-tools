#include <dsfw/JsonHelper.h>
#include <dsfw/PathUtils.h>
#include <dstools/ExportFormats.h>
#include <filesystem>
#include <fstream>

namespace dstools {

    const char *HtsLabelExportFormat::formatName() const {
        return "HTS Labels";
    }
    const char *HtsLabelExportFormat::formatExtension() const {
        return ".lab";
    }

    Result<void> HtsLabelExportFormat::exportItem(const std::filesystem::path &sourceFile,
                                                  const std::filesystem::path &workingDir,
                                                  const std::filesystem::path &outputPath) {
        auto jsonResult = JsonHelper::loadFile(sourceFile);
        if (!jsonResult)
            return Err(jsonResult.error());

        const auto &j = jsonResult.value();
        std::ofstream out(outputPath);
        if (!out.is_open()) {
            return Err("Cannot open output file: " + dsfw::PathUtils::toUtf8(outputPath));
        }

        (void) workingDir;

        if (j.contains("phonemes") && j["phonemes"].is_array()) {
            for (const auto &ph : j["phonemes"]) {
                out << ph.value("start", 0.0) << " " << ph.value("end", 0.0) << " " << ph.value("symbol", "")
                    << "\n";
                if (!out.good())
                    return Err("Write error in HTS label export");
            }
        }
        return Ok();
    }

    Result<void> HtsLabelExportFormat::exportAll(const std::filesystem::path &workingDir,
                                                 const std::filesystem::path &outputDir) {
        std::error_code ec;
        for (const auto &entry : std::filesystem::directory_iterator(workingDir, ec)) {
            if (entry.path().extension() == ".dsitem") {
                auto dst = outputDir / entry.path().stem();
                dst += ".lab";
                auto result = exportItem(entry.path(), workingDir, dst);
                if (!result)
                    return result;
            }
        }
        if (ec)
            return Err("Failed to scan working directory: " + ec.message());
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
            return Err("Cannot open output file: " + dsfw::PathUtils::toUtf8(outputPath));
        }

        (void) workingDir;

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        out << "<sinsy>\n";
        if (j.contains("phonemes") && j["phonemes"].is_array()) {
            for (const auto &ph : j["phonemes"]) {
                out << "  <ph start=\"" << ph.value("start", 0.0) << "\" end=\"" << ph.value("end", 0.0)
                    << "\" symbol=\"" << ph.value("symbol", "") << "\"/>\n";
                if (!out.good())
                    return Err("Write error in Sinsy XML export");
            }
        }
        out << "</sinsy>\n";
        if (!out.good())
            return Err("Write error in Sinsy XML export");
        return Ok();
    }

    const char *SinsyXmlExportFormat::formatName() const {
        return "Sinsy XML";
    }
    const char *SinsyXmlExportFormat::formatExtension() const {
        return ".xml";
    }

    Result<void> SinsyXmlExportFormat::exportAll(const std::filesystem::path &workingDir,
                                                 const std::filesystem::path &outputDir) {
        std::error_code ec;
        for (const auto &entry : std::filesystem::directory_iterator(workingDir, ec)) {
            if (entry.path().extension() == ".dsitem") {
                auto dst = outputDir / entry.path().stem();
                dst += ".xml";
                auto result = exportItem(entry.path(), workingDir, dst);
                if (!result)
                    return result;
            }
        }
        if (ec)
            return Err("Failed to scan working directory: " + ec.message());
        return Ok();
    }

} // namespace dstools
