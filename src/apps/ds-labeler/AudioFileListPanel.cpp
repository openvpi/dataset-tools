#include "AudioFileListPanel.h"

namespace dstools {

AudioFileListPanel::AudioFileListPanel(QWidget *parent)
    : DroppableFileListPanel(parent) {
    setFileFilters({
        QStringLiteral("*.wav"),
        QStringLiteral("*.flac"),
        QStringLiteral("*.mp3"),
        QStringLiteral("*.m4a"),
        QStringLiteral("*.ogg"),
        QStringLiteral("*.opus"),
        QStringLiteral("*.wma"),
        QStringLiteral("*.aac"),
    });
}

} // namespace dstools
