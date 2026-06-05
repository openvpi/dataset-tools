#include <dsfw/FileUtils.h>

#include <QDir>
#include <dsfw/AtomicFileWriter.h>
#include <dsfw/PathUtils.h>

#include <QFile>
#include <QFileInfo>

namespace dsfw {

    dsfw::Result<QByteArray> FileUtils::readAll(const std::filesystem::path &path) {
        QFile file(PathUtils::fromStdPath(path));
        if (!file.open(QIODevice::ReadOnly)) {
            return dsfw::Result<QByteArray>::Error("FileUtils::readAll: cannot open file: " + PathUtils::toUtf8(path));
        }
        const QByteArray data = file.readAll();
        file.close();
        return data;
    }

    dsfw::Result<void> FileUtils::writeAll(const std::filesystem::path &path, const QByteArray &data) {
        std::string content(data.constData(), static_cast<size_t>(data.size()));
        return AtomicFileWriter::write(path, content);
    }

    dsfw::Result<void> FileUtils::copy(const std::filesystem::path &src, const std::filesystem::path &dst) {
        const QString qSrc = PathUtils::fromStdPath(src);
        const QString qDst = PathUtils::fromStdPath(dst);

        if (!QFile::exists(qSrc)) {
            return dsfw::Result<void>::Error("FileUtils::copy: source does not exist: " + PathUtils::toUtf8(src));
        }

        // Remove destination if it already exists
        if (QFile::exists(qDst)) {
            if (!QFile::remove(qDst)) {
                return dsfw::Result<void>::Error("FileUtils::copy: cannot overwrite destination: " +
                                                    PathUtils::toUtf8(dst));
            }
        }

        // Ensure parent directory exists
        QFileInfo dstInfo(qDst);
        QDir().mkpath(dstInfo.absolutePath());

        if (!QFile::copy(qSrc, qDst)) {
            return dsfw::Result<void>::Error("FileUtils::copy: failed to copy from " + PathUtils::toUtf8(src) +
                                                " to " + PathUtils::toUtf8(dst));
        }

        return dsfw::Result<void>::Ok();
    }

    bool FileUtils::exists(const std::filesystem::path &path) {
        std::error_code ec;
        bool result = std::filesystem::exists(path, ec);
        if (ec) {
            return false;
        }
        return result && std::filesystem::is_regular_file(path, ec);
    }

    dsfw::Result<uint64_t> FileUtils::fileSize(const std::filesystem::path &path) {
        std::error_code ec;
        auto size = std::filesystem::file_size(path, ec);
        if (ec) {
            return dsfw::Result<uint64_t>::Error("FileUtils::fileSize: " + ec.message() + ": " +
                                                    PathUtils::toUtf8(path));
        }
        return static_cast<uint64_t>(size);
    }

} // namespace dsfw