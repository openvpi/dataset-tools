/// @file SliceExportDialog.h
/// @brief Dialog for configuring slice audio export options.

#pragma once

#include <QDialog>

class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QSpinBox;

namespace dstools {

/// @brief Export configuration dialog for sliced audio.
///
/// Allows user to select bit depth, channel mode, output directory,
/// naming prefix, and sequence number digits.
class SliceExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit SliceExportDialog(QWidget *parent = nullptr);

    /// Bit depth options.
    enum class BitDepth { PCM16, PCM24, PCM32, Float32 };

    /// Channel mode options.
    enum class ChannelExportMode { Mono, KeepOriginal };

    [[nodiscard]] BitDepth bitDepth() const;
    [[nodiscard]] ChannelExportMode channelMode() const;
    [[nodiscard]] QString outputDir() const;
    [[nodiscard]] QString prefix() const;
    [[nodiscard]] int numDigits() const;

    void setDefaultOutputDir(const QString &dir);
    void setDefaultPrefix(const QString &prefix);

private:
    QComboBox *m_comboBitDepth = nullptr;
    QComboBox *m_comboChannel = nullptr;
    QLineEdit *m_editOutputDir = nullptr;
    QLineEdit *m_editPrefix = nullptr;
    QSpinBox *m_spinDigits = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;

    void onBrowseDir();
};

} // namespace dstools
