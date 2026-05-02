/// @file ExportDialog.h
/// @brief MinLabel dataset export dialog.

#pragma once
#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <dstools/PathSelector.h>

#include "Common.h"

namespace Minlabel {
    /// @brief Dialog for configuring dataset export settings (output dir, filename conversion, audio/label/text options).
    class ExportDialog final : public QDialog {
        Q_OBJECT
    public:
        /// @brief Constructs the export dialog.
        /// @param parent Optional parent widget.
        explicit ExportDialog(QWidget *parent = nullptr);
        ~ExportDialog() override;

        ExportInfo exportInfo;  ///< Collected export settings.

        dstools::widgets::PathSelector *m_outputDir;  ///< Output directory selector.

        QLineEdit *folderNameEdit;       ///< Subfolder name input.
        QCheckBox *convertFilename{};    ///< Option to convert filenames.
        QCheckBox *expAudio;             ///< Option to export audio files.
        QCheckBox *labFile;              ///< Option to export label files.
        QCheckBox *rawText;              ///< Option to export raw text files.
        QCheckBox *removeTone;           ///< Option to remove tone markers.
    };
}
