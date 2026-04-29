#include <dstools/LocalFileIOProvider.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include <memory>

namespace dstools {

// ── Binary I/O ───────────────────────────────────────────────────────

QByteArray LocalFileIOProvider::readFile(const QString &path, std::string &error) {
    error.clear();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        error = f.errorString().toStdString();
        return {};
    }
    return f.readAll();
}

bool LocalFileIOProvider::writeFile(const QString &path, const QByteArray &data,
                                    std::string &error) {
    error.clear();
    // Ensure parent directory exists.
    const QFileInfo fi(path);
    if (!QDir().mkpath(fi.absolutePath())) {
        error = "Failed to create directory: " + fi.absolutePath().toStdString();
        return false;
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = f.errorString().toStdString();
        return false;
    }
    if (f.write(data) != data.size()) {
        error = f.errorString().toStdString();
        return false;
    }
    return true;
}

// ── Text I/O ─────────────────────────────────────────────────────────

QString LocalFileIOProvider::readText(const QString &path, std::string &error) {
    const QByteArray bytes = readFile(path, error);
    if (!error.empty())
        return {};
    return QString::fromUtf8(bytes);
}

bool LocalFileIOProvider::writeText(const QString &path, const QString &text,
                                    std::string &error) {
    return writeFile(path, text.toUtf8(), error);
}

// ── Queries ──────────────────────────────────────────────────────────

bool LocalFileIOProvider::exists(const QString &path) {
    return QFileInfo::exists(path);
}

FileInfo LocalFileIOProvider::fileInfo(const QString &path) {
    const QFileInfo qi(path);
    FileInfo info;
    info.path = qi.absoluteFilePath();
    info.size = qi.size();
    info.lastModified = qi.lastModified();
    info.exists = qi.exists();
    info.isDirectory = qi.isDir();
    return info;
}

QStringList LocalFileIOProvider::listFiles(const QString &directory,
                                           const QStringList &nameFilters, bool recursive,
                                           std::string &error) {
    error.clear();
    const QDir dir(directory);
    if (!dir.exists()) {
        error = "Directory does not exist: " + directory.toStdString();
        return {};
    }

    const auto flags =
        recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    QDirIterator it(directory, nameFilters, QDir::Files, flags);

    QStringList result;
    while (it.hasNext())
        result.append(it.next());
    return result;
}

// ── Mutations ────────────────────────────────────────────────────────

bool LocalFileIOProvider::mkdirs(const QString &path, std::string &error) {
    error.clear();
    if (!QDir().mkpath(path)) {
        error = "Failed to create directory: " + path.toStdString();
        return false;
    }
    return true;
}

bool LocalFileIOProvider::removeFile(const QString &path, std::string &error) {
    error.clear();
    if (!QFile::remove(path)) {
        error = "Failed to remove file: " + path.toStdString();
        return false;
    }
    return true;
}

bool LocalFileIOProvider::copyFile(const QString &src, const QString &dst,
                                   std::string &error) {
    error.clear();
    // QFile::copy fails if dst exists, so remove it first.
    if (QFileInfo::exists(dst))
        QFile::remove(dst);
    if (!QFile::copy(src, dst)) {
        error = "Failed to copy " + src.toStdString() + " to " + dst.toStdString();
        return false;
    }
    return true;
}

// ── Global provider ──────────────────────────────────────────────────

static std::unique_ptr<IFileIOProvider> s_provider =
    std::make_unique<LocalFileIOProvider>();

IFileIOProvider *fileIOProvider() {
    return s_provider.get();
}

void setFileIOProvider(IFileIOProvider *provider) {
    s_provider.reset(provider);
}

} // namespace dstools
