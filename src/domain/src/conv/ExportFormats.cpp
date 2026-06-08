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

    dsfw::Result<void> HtsLabelExportFormat::exportItem(const std::filesystem::path &sourceFile,
                                                  const std::filesystem::path &workingDir,
                                                  const std::filesystem::path &outputPath) {
        auto jsonResult = JsonHelper::loadFile(sourceFile);
        if (!jsonResult)
            return dsfw::Err(jsonResult.error());

        const auto &j = jsonResult.value();
        std::ofstream out(outputPath);
        if (!out.is_open()) {
            return dsfw::Err("Cannot open output file: " + dsfw::PathUtils::toUtf8(outputPath));
        }

        (void) workingDir;

        if (j.contains("phonemes") && j["phonemes"].is_array()) {
            for (const auto &ph : j["phonemes"]) {
                out << ph.value("start", 0.0) << " " << ph.value("end", 0.0) << " " << ph.value("symbol", "")
                    << "\n";
                if (!out.good())
                    return dsfw::Err("Write error in HTS label export");
            }
        }
        return dsfw::Ok();
    }

    dsfw::Result<void> HtsLabelExportFormat::exportAll(const std::filesystem::path &workingDir,
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
            return dsfw::Err("Failed to scan working directory: " + ec.message());
        return dsfw::Ok();
    }

    dsfw::Result<void> SinsyXmlExportFormat::exportItem(const std::filesystem::path &sourceFile,
                                                  const std::filesystem::path &workingDir,
                                                  const std::filesystem::path &outputPath) {
        auto jsonResult = JsonHelper::loadFile(sourceFile);
        if (!jsonResult)
            return dsfw::Err(jsonResult.error());

        const auto &j = jsonResult.value();
        std::ofstream out(outputPath);
        if (!out.is_open()) {
            return dsfw::Err("Cannot open output file: " + dsfw::PathUtils::toUtf8(outputPath));
        }

        (void) workingDir;

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        out << "<sinsy>\n";
        if (j.contains("phonemes") && j["phonemes"].is_array()) {
            for (const auto &ph : j["phonemes"]) {
                out << "  <ph start=\"" << ph.value("start", 0.0) << "\" end=\"" << ph.value("end", 0.0)
                    << "\" symbol=\"" << ph.value("symbol", "") << "\"/>\n";
                if (!out.good())
                    return dsfw::Err("Write error in Sinsy XML export");
            }
        }
        out << "</sinsy>\n";
        if (!out.good())
            return dsfw::Err("Write error in Sinsy XML export");
        return dsfw::Ok();
    }

    const char *SinsyXmlExportFormat::formatName() const {
        return "Sinsy XML";
    }
    const char *SinsyXmlExportFormat::formatExtension() const {
        return ".xml";
    }

    dsfw::Result<void> SinsyXmlExportFormat::exportAll(const std::filesystem::path &workingDir,
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
            return dsfw::Err("Failed to scan working directory: " + ec.message());
        return dsfw::Ok();
    }

} // namespace dstools
