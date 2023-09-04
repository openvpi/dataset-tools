#include "ExportDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>


ExportDialog::ExportDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Export");

    auto *layout = new QFormLayout(this);

    dirnameEdit = new QLineEdit(this);
    dirnameEdit->setText(QDir::currentPath());
    dirnameButton = new QPushButton("...", this);
    dirnameButton->setMaximumWidth(100);

    connect(dirnameButton, &QPushButton::clicked, this, [=]() {
        QString dirname = QFileDialog::getExistingDirectory(this, "Select Directory", dirnameEdit->text());
        if (!dirname.isEmpty()) {
            dirnameEdit->setText(dirname);
        }
    });

    auto hLayout = new QHBoxLayout();
    hLayout->addWidget(dirnameEdit);
    hLayout->addWidget(dirnameButton);

    layout->addRow(new QLabel("Directory:", this));
    layout->addRow(hLayout);

    outputDirEdit = new QLineEdit(this);
    outputDirEdit->setText("minlabel_export_audio");
    layout->addRow(new QLabel("Output Folder Name:", this), outputDirEdit);

    convertFilename = new QCheckBox("Convert chinese in filename to pinyin", this);
    layout->addRow(convertFilename);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(this, &QDialog::accepted, this, [=]() {
        dirPath = dirnameEdit->text();
        outputDir = outputDirEdit->text();
        convertPinyin = convertFilename->isChecked();
    });

    resize(400, 200);
}

ExportDialog::~ExportDialog() {
}
