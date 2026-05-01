/// @file GameAlignPage.h
/// @brief GAME alignment page for the labeler wizard.

#pragma once

#include <QWidget>

class QLineEdit;
class QComboBox;
class ProgressWidget;

/// @brief Page for running GAME-based note alignment on audio files.
class GameAlignPage : public QWidget {
    Q_OBJECT
public:
    /// @brief Constructs the GAME alignment page.
    /// @param parent Optional parent widget.
    explicit GameAlignPage(QWidget *parent = nullptr);

private slots:
    void onRunAlignment();  ///< Starts the alignment process.

private:
    QLineEdit *m_modelPath;       ///< GAME model directory path.
    QComboBox *m_gpuSelector;     ///< GPU device selector.
    ProgressWidget *m_runProgress; ///< Progress indicator widget.
    QWidget *m_log;               ///< Log output area.
};
