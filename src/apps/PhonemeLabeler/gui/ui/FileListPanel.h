#pragma once

#include <dstools/BaseFileListPanel.h>

namespace dstools {
namespace phonemelabeler {

class FileListPanel : public dstools::widgets::BaseFileListPanel {
    Q_OBJECT
public:
    explicit FileListPanel(QWidget *parent = nullptr);

    [[nodiscard]] QStringList textGridFiles() const;
};

} // namespace phonemelabeler
} // namespace dstools
