#include "ExportDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>

ExportDialog::ExportDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Export");

    auto *layout = new QFormLayout(this);

    outputDirEdit = new QLineEdit(this);
    outputDirEdit->setText(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    outputDirButton = new QPushButton("...", this);
    outputDirButton->setMaximumWidth(100);

    connect(outputDirButton, &QPushButton::clicked, this, [=] {
        const QString dirname = QFileDialog::getExistingDirectory(this, "Select Directory", outputDirEdit->text());
        if (!dirname.isEmpty()) {
            outputDirEdit->setText(dirname);
        }
    });

    const auto hLayout = new QHBoxLayout();
    hLayout->addWidget(outputDirEdit);
    hLayout->addWidget(outputDirButton);

    layout->addRow(new QLabel("Out Directory:", this));
    layout->addRow(hLayout);

    folderNameEdit = new QLineEdit(this);
    folderNameEdit->setText("minlabel_export");
    layout->addRow(new QLabel("Output Folder Name:", this), folderNameEdit);

    expAudio = new QCheckBox("Export audio", this);
    expAudio->setChecked(true);
    layout->addRow(expAudio);

    labFile = new QCheckBox("Export word sequences(*.lab)", this);
    labFile->setChecked(true);
    layout->addRow(labFile);

    rawText = new QCheckBox("Export raw text(*.txt)", this);
    layout->addRow(rawText);

    removeTone = new QCheckBox("Export word sequences without tone(*.lab)", this);
    layout->addRow(removeTone);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(this, &QDialog::accepted, this, [=] {
        exportInfo.outputDir = outputDirEdit->text();
        exportInfo.folderName = folderNameEdit->text();
        exportInfo.exportAudio = expAudio->isChecked();
        exportInfo.labFile = labFile->isChecked();
        exportInfo.rawText = rawText->isChecked();
        exportInfo.removeTone = removeTone->isChecked();
    });

    resize(500, 300);
}

ExportDialog::~ExportDialog() = default;
