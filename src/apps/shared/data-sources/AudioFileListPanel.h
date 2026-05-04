/// @file AudioFileListPanel.h
/// @brief Audio file list panel for DsSlicerPage — thin wrapper around DroppableFileListPanel.

#pragma once

#include <dsfw/widgets/DroppableFileListPanel.h>

namespace dstools {

/// @brief Audio file list for the slicer page.
///
/// Pre-configured DroppableFileListPanel with audio file filters
/// (wav, flac, mp3, m4a, ogg, opus, wma, aac).
class AudioFileListPanel : public dsfw::widgets::DroppableFileListPanel {
    Q_OBJECT

public:
    explicit AudioFileListPanel(QWidget *parent = nullptr);
    ~AudioFileListPanel() override = default;
};

} // namespace dstools
