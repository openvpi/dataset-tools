/// @file NewProjectDialog.h
/// @brief Modal dialog for creating a new DsLabeler project.

#pragma once

#include <QDialog>

class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace dstools {

/// @brief New project creation dialog.
///
/// Collects project name, save location, audio files, language, and speaker.
/// Creates a .dsproj file on accept.
class NewProjectDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget *parent = nullptr);
    ~NewProjectDialog() override = default;

    /// Return the path to the created .dsproj file. Empty if cancelled.
    [[nodiscard]] QString projectFilePath() const { return m_projectFilePath; }

private:
    QLineEdit *m_editName = nullptr;
    QLabel *m_labelDir = nullptr;
    QPushButton *m_btnBrowseDir = nullptr;
    QComboBox *m_comboLanguage = nullptr;
    QLineEdit *m_editSpeaker = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;

    QString m_saveDir;
    QString m_projectFilePath;

    void onBrowseDir();
    void onAccepted();
    void updateOkButton();
};

} // namespace dstools
