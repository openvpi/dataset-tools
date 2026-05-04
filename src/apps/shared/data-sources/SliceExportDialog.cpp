#include "SliceExportDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace dstools {

SliceExportDialog::SliceExportDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Export Slice Audio"));
    setMinimumWidth(400);

    auto *layout = new QFormLayout(this);

    m_comboBitDepth = new QComboBox(this);
    m_comboBitDepth->addItem(tr("16-bit PCM"), static_cast<int>(BitDepth::PCM16));
    m_comboBitDepth->addItem(tr("24-bit PCM"), static_cast<int>(BitDepth::PCM24));
    m_comboBitDepth->addItem(tr("32-bit PCM"), static_cast<int>(BitDepth::PCM32));
    m_comboBitDepth->addItem(tr("32-bit float"), static_cast<int>(BitDepth::Float32));
    layout->addRow(tr("Bit depth:"), m_comboBitDepth);

    m_comboChannel = new QComboBox(this);
    m_comboChannel->addItem(tr("Mono"), static_cast<int>(ChannelExportMode::Mono));
    m_comboChannel->addItem(tr("Keep original"), static_cast<int>(ChannelExportMode::KeepOriginal));
    layout->addRow(tr("Channels:"), m_comboChannel);

    auto *dirLayout = new QHBoxLayout;
    m_editOutputDir = new QLineEdit(this);
    auto *btnBrowse = new QPushButton(tr("Browse..."), this);
    dirLayout->addWidget(m_editOutputDir, 1);
    dirLayout->addWidget(btnBrowse);
    layout->addRow(tr("Output dir:"), dirLayout);
    connect(btnBrowse, &QPushButton::clicked, this, &SliceExportDialog::onBrowseDir);

    m_editPrefix = new QLineEdit(this);
    layout->addRow(tr("Prefix:"), m_editPrefix);

    m_spinDigits = new QSpinBox(this);
    m_spinDigits->setRange(1, 10);
    m_spinDigits->setValue(3);
    layout->addRow(tr("Number digits:"), m_spinDigits);

    m_buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Export"));
    layout->addRow(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SliceExportDialog::BitDepth SliceExportDialog::bitDepth() const {
    return static_cast<BitDepth>(m_comboBitDepth->currentData().toInt());
}

SliceExportDialog::ChannelExportMode SliceExportDialog::channelMode() const {
    return static_cast<ChannelExportMode>(m_comboChannel->currentData().toInt());
}

QString SliceExportDialog::outputDir() const {
    return m_editOutputDir->text();
}

QString SliceExportDialog::prefix() const {
    return m_editPrefix->text();
}

int SliceExportDialog::numDigits() const {
    return m_spinDigits->value();
}

void SliceExportDialog::setDefaultOutputDir(const QString &dir) {
    m_editOutputDir->setText(dir);
}

void SliceExportDialog::setDefaultPrefix(const QString &prefix) {
    m_editPrefix->setText(prefix);
}

void SliceExportDialog::onBrowseDir() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"));
    if (!dir.isEmpty())
        m_editOutputDir->setText(dir);
}

} // namespace dstools
