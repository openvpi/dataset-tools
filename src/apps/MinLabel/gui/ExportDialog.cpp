#include "ExportDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>

namespace Minlabel {
    ExportDialog::ExportDialog(QWidget *parent) : QDialog(parent) {
        setWindowTitle("Export");

        auto *layout = new QFormLayout(this);

        m_outputDir = new dstools::widgets::PathSelector(dstools::widgets::PathSelector::Directory,
                                                         "", "", this);
        m_outputDir->setPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));

        layout->addRow(new QLabel("Out Directory:", this));
        layout->addRow(m_outputDir);

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
            exportInfo.outputDir = m_outputDir->path();
            exportInfo.folderName = folderNameEdit->text();
            exportInfo.exportAudio = expAudio->isChecked();
            exportInfo.labFile = labFile->isChecked();
            exportInfo.rawText = rawText->isChecked();
            exportInfo.removeTone = removeTone->isChecked();
        });

        resize(500, 300);
    }

    ExportDialog::~ExportDialog() = default;
}
