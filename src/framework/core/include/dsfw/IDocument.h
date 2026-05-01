#pragma once

#include <dsfw/IDocumentFormat.h>
#include <dstools/Result.h>

#include <filesystem>
#include <string>

namespace dstools {

class IDocument {
public:
    virtual ~IDocument() = default;

    virtual bool isModified() const = 0;

    virtual Result<void> load(const std::filesystem::path &path) = 0;
    virtual Result<void> save() = 0;
    virtual Result<void> saveAs(const std::filesystem::path &path) = 0;

    virtual IDocumentFormat *format() const = 0;
};

} // namespace dstools
