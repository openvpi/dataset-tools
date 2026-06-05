#pragma once

/// @file IDocument.h
/// @brief Document model interface for load/save operations.

#include <dstools/Result.h>
#include <filesystem>
#include <string>

namespace dsfw {

    /// @brief Abstract document interface supporting modified tracking and file persistence.
    class IDocument {
    public:
        virtual ~IDocument() noexcept = default;

        /// @brief Check whether the document has unsaved changes.
        /// @return True if modified.
        virtual bool isModified() const = 0;

        /// @brief Load document from a file.
        /// @param path File path to load from.
        /// @return Success or error.
        [[nodiscard]] virtual Result<void> load(const std::filesystem::path &path) = 0;

        /// @brief Save document to the current path.
        /// @return Success or error.
        [[nodiscard]] virtual Result<void> save() = 0;

        /// @brief Save document to a new path.
        /// @param path Destination file path.
        /// @return Success or error.
        [[nodiscard]] virtual Result<void> saveAs(const std::filesystem::path &path) = 0;
    };

} // namespace dsfw

// Backward compatibility
namespace dstools {
    using dsfw::IDocument;
}
