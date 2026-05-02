/// @file CleanupDialog.h
/// @brief Step cleanup dialog for the DiffSingerLabeler wizard.

#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QVector>

namespace dstools::labeler {

/// @brief Dialog listing completed pipeline steps with checkboxes for selective cleanup.
class CleanupDialog : public QDialog {
    Q_OBJECT
public:
    /// @brief Constructs the cleanup dialog.
    /// @param workingDir Path to the working directory to scan for steps.
    /// @param parent Optional parent widget.
    explicit CleanupDialog(const QString &workingDir, QWidget *parent = nullptr);

    /// @brief Returns the 0-based indices of steps selected for cleanup.
    /// @return Vector of selected step indices.
    QVector<int> selectedSteps() const; // 0-based step indices

private:
    QString m_workingDir;                ///< Working directory path.
    QVector<QCheckBox *> m_checkBoxes;   ///< Checkboxes for each discovered step.

    void buildLayout();    ///< Builds the dialog layout.
    void scanDirectory();  ///< Scans the working directory for completed steps.
    void selectAll();      ///< Checks all step checkboxes.
    void deselectAll();    ///< Unchecks all step checkboxes.
};

} // namespace dstools::labeler
