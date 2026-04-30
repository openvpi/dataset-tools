#include <dstools/LocalFileIOProvider.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include <memory>

namespace dstools {

Result<QByteArray> LocalFileIOProvider::readFile(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return Result<QByteArray>::Error(f.errorString().toStdString());
    }
    return Result<QByteArray>::Ok(f.readAll());
}

Result<void> LocalFileIOProvider::writeFile(const QString &path, const QByteArray &data) {
    const QFileInfo fi(path);
    if (!QDir().mkpath(fi.absolutePath())) {
        return Result<void>::Error("Failed to create directory: " + fi.absolutePath().toStdString());
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return Result<void>::Error(f.errorString().toStdString());
    }
    if (f.write(data) != data.size()) {
        return Result<void>::Error(f.errorString().toStdString());
    }
    return Result<void>::Ok();
}

Result<QString> LocalFileIOProvider::readText(const QString &path) {
    auto bytesResult = readFile(path);
    if (!bytesResult)
        return Result<QString>::Error(bytesResult.error());
    return Result<QString>::Ok(QString::fromUtf8(bytesResult.value()));
}

Result<void> LocalFileIOProvider::writeText(const QString &path, const QString &text) {
    return writeFile(path, text.toUtf8());
}

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

Result<QStringList> LocalFileIOProvider::listFiles(const QString &directory,
                                                   const QStringList &nameFilters, bool recursive) {
    const QDir dir(directory);
    if (!dir.exists()) {
        return Result<QStringList>::Error("Directory does not exist: " + directory.toStdString());
    }

    const auto flags =
        recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    QDirIterator it(directory, nameFilters, QDir::Files, flags);

    QStringList result;
    while (it.hasNext())
        result.append(it.next());
    return Result<QStringList>::Ok(result);
}

Result<void> LocalFileIOProvider::mkdirs(const QString &path) {
    if (!QDir().mkpath(path)) {
        return Result<void>::Error("Failed to create directory: " + path.toStdString());
    }
    return Result<void>::Ok();
}

Result<void> LocalFileIOProvider::removeFile(const QString &path) {
    if (!QFile::remove(path)) {
        return Result<void>::Error("Failed to remove file: " + path.toStdString());
    }
    return Result<void>::Ok();
}

Result<void> LocalFileIOProvider::copyFile(const QString &src, const QString &dst) {
    if (QFileInfo::exists(dst))
        QFile::remove(dst);
    if (!QFile::copy(src, dst)) {
        return Result<void>::Error("Failed to copy " + src.toStdString() + " to " + dst.toStdString());
    }
    return Result<void>::Ok();
}

static std::unique_ptr<IFileIOProvider> s_provider =
    std::make_unique<LocalFileIOProvider>();

IFileIOProvider *fileIOProvider() {
    return s_provider.get();
}

void setFileIOProvider(IFileIOProvider *provider) {
    s_provider.reset(provider);
}

void resetFileIOProvider() {
    s_provider = std::make_unique<LocalFileIOProvider>();
}

}
