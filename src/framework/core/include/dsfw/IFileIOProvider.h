#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QStringList>

#include <dstools/Result.h>

#include <vector>

namespace dstools {

struct FileInfo {
    QString path;
    qint64 size = 0;
    QDateTime lastModified;
    bool exists = false;
    bool isDirectory = false;
};

class IFileIOProvider {
public:
    virtual ~IFileIOProvider() = default;

    virtual Result<QByteArray> readFile(const QString &path) = 0;
    virtual Result<void> writeFile(const QString &path, const QByteArray &data) = 0;

    virtual Result<QString> readText(const QString &path) = 0;
    virtual Result<void> writeText(const QString &path, const QString &text) = 0;

    virtual bool exists(const QString &path) = 0;
    virtual FileInfo fileInfo(const QString &path) = 0;
    virtual Result<QStringList> listFiles(const QString &directory, const QStringList &nameFilters,
                                          bool recursive) = 0;

    virtual Result<void> mkdirs(const QString &path) = 0;
    virtual Result<void> removeFile(const QString &path) = 0;
    virtual Result<void> copyFile(const QString &src, const QString &dst) = 0;
};

}
