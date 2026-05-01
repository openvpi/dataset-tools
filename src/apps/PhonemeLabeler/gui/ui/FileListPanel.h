/// @file FileListPanel.h
/// @brief TextGrid file list panel for PhonemeLabeler.

#pragma once

#include <dstools/BaseFileListPanel.h>

namespace dstools {
namespace phonemelabeler {

/// @brief BaseFileListPanel subclass listing TextGrid files in the working directory.
class FileListPanel : public dstools::widgets::BaseFileListPanel {
    Q_OBJECT
public:
    /// @brief Constructs the file list panel.
    /// @param parent Optional parent widget.
    explicit FileListPanel(QWidget *parent = nullptr);

    /// @brief Returns the list of TextGrid file paths in the current directory.
    /// @return List of TextGrid file paths.
    [[nodiscard]] QStringList textGridFiles() const;
};

} // namespace phonemelabeler
} // namespace dstools
