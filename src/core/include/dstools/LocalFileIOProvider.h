#pragma once

/// @file LocalFileIOProvider.h
/// @brief Local filesystem implementation of IFileIOProvider.

#include <dstools/IFileIOProvider.h>

namespace dstools {

/// IFileIOProvider implementation backed by the local filesystem.
class LocalFileIOProvider final : public IFileIOProvider {
public:
    QByteArray readFile(const QString &path, std::string &error) override;
    bool writeFile(const QString &path, const QByteArray &data, std::string &error) override;

    QString readText(const QString &path, std::string &error) override;
    bool writeText(const QString &path, const QString &text, std::string &error) override;

    bool exists(const QString &path) override;
    FileInfo fileInfo(const QString &path) override;
    QStringList listFiles(const QString &directory, const QStringList &nameFilters,
                          bool recursive, std::string &error) override;

    bool mkdirs(const QString &path, std::string &error) override;
    bool removeFile(const QString &path, std::string &error) override;
    bool copyFile(const QString &src, const QString &dst, std::string &error) override;
};

/// Return the current global IFileIOProvider (defaults to LocalFileIOProvider).
IFileIOProvider *fileIOProvider();

/// Replace the global IFileIOProvider. Takes ownership of @p provider.
void setFileIOProvider(IFileIOProvider *provider);

} // namespace dstools
