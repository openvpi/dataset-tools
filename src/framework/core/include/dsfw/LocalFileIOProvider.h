#pragma once

#include <dsfw/IFileIOProvider.h>

namespace dstools {

class LocalFileIOProvider final : public IFileIOProvider {
public:
    Result<QByteArray> readFile(const QString &path) override;
    Result<void> writeFile(const QString &path, const QByteArray &data) override;

    Result<QString> readText(const QString &path) override;
    Result<void> writeText(const QString &path, const QString &text) override;

    bool exists(const QString &path) override;
    FileInfo fileInfo(const QString &path) override;
    Result<QStringList> listFiles(const QString &directory, const QStringList &nameFilters,
                                  bool recursive) override;

    Result<void> mkdirs(const QString &path) override;
    Result<void> removeFile(const QString &path) override;
    Result<void> copyFile(const QString &src, const QString &dst) override;
};

IFileIOProvider *fileIOProvider();
void setFileIOProvider(IFileIOProvider *provider);
void resetFileIOProvider();

}
