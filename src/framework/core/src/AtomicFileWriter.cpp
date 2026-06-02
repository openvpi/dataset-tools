#include <dsfw/AtomicFileWriter.h>
#include <dsfw/PathUtils.h>

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <nlohmann/json.hpp>

namespace dsfw {

    std::atomic<bool> AtomicFileWriter::s_backupEnabled{true};
    std::atomic<bool> AtomicFileWriter::s_validationEnabled{true};

    dstools::Result<void> AtomicFileWriter::write(const std::filesystem::path &path, const std::string &content) {
        return writeImpl(path, content, false);
    }

    dstools::Result<void> AtomicFileWriter::writeJson(const std::filesystem::path &path, const std::string &jsonContent) {
        return writeImpl(path, jsonContent, true);
    }

    void AtomicFileWriter::setBackupEnabled(bool enabled) { s_backupEnabled.store(enabled); }
    bool AtomicFileWriter::isBackupEnabled() noexcept { return s_backupEnabled.load(); }

    void AtomicFileWriter::setValidationEnabled(bool enabled) { s_validationEnabled.store(enabled); }
    bool AtomicFileWriter::isValidationEnabled() noexcept { return s_validationEnabled.load(); }

    dstools::Result<void> AtomicFileWriter::writeImpl(const std::filesystem::path &path,
                                                      const std::string &content,
                                                      bool validateJson) {
        const QString qPath = dsfw::PathUtils::fromStdPath(path);

        if (s_backupEnabled.load()) {
            const QString backupPath = qPath + QStringLiteral(".bak");
            if (QFile::exists(qPath))
                QFile::copy(qPath, backupPath);
        }

        QDir().mkpath(QFileInfo(qPath).absolutePath());

        QSaveFile file(qPath);
        if (!file.open(QIODevice::WriteOnly))
            return dstools::Result<void>::Error("AtomicFileWriter: cannot open for writing: " + dsfw::PathUtils::toUtf8(path));

        const qint64 written = file.write(content.data(), static_cast<qint64>(content.size()));
        if (written != static_cast<qint64>(content.size())) {
            file.cancelWriting();
            return dstools::Result<void>::Error("AtomicFileWriter: write incomplete: " + dsfw::PathUtils::toUtf8(path));
        }

        if (!file.commit())
            return dstools::Result<void>::Error("AtomicFileWriter: commit failed: " + dsfw::PathUtils::toUtf8(path));

        if (validateJson && s_validationEnabled.load()) {
            try {
                nlohmann::json::parse(content, nullptr, true);
            } catch (const std::exception &e) {
                const QString backupPath = qPath + QStringLiteral(".bak");
                if (QFile::exists(backupPath)) {
                    QFile::remove(qPath);
                    QFile::copy(backupPath, qPath);
                }
                return dstools::Result<void>::Error(
                    std::string("AtomicFileWriter: JSON validation failed: ") + e.what());
            }
        }

        return dstools::Result<void>::Ok();
    }

} // namespace dsfw