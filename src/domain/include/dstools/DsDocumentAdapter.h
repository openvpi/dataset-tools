#pragma once

/// @file DsDocumentAdapter.h
/// @brief Adapter bridging DsDocument to the IDocument interface.

#include <dsfw/IDocument.h>
#include <dstools/DsDocument.h>

#include <memory>

namespace dstools {

/// @brief Wraps a DsDocument reference to satisfy the IDocument interface,
///        delegating load/save/modified operations.
class DsDocumentAdapter : public IDocument {
public:
    /// @brief Construct an adapter around the given document.
    /// @param doc Reference to the DsDocument to wrap.
    explicit DsDocumentAdapter(DsDocument &doc);

    /// @brief Check whether the underlying document has unsaved changes.
    /// @return True if the document has been modified.
    bool isModified() const override;

    /// @brief Load document content from disk.
    /// @param path File path to load from.
    /// @return Success or an error description.
    Result<void> load(const std::filesystem::path &path) override;

    /// @brief Save the document to its current path.
    /// @return Success or an error description.
    Result<void> save() override;

    /// @brief Save the document to a new path.
    /// @param path Destination file path.
    /// @return Success or an error description.
    Result<void> saveAs(const std::filesystem::path &path) override;

private:
    DsDocument &m_doc; ///< Wrapped document reference.
};

} // namespace dstools
