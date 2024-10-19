#ifndef DATASET_TOOLS_EXPORTDIALOG_H
#define DATASET_TOOLS_EXPORTDIALOG_H


#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>

#include "Common.h"

class ExportDialog final : public QDialog {
    Q_OBJECT
public:
    explicit ExportDialog(QWidget *parent = nullptr);
    ~ExportDialog() override;

    ExportInfo exportInfo;

    QLineEdit *outputDirEdit;
    QPushButton *outputDirButton;

    QLineEdit *folderNameEdit;
    QCheckBox *convertFilename{};
    QCheckBox *expAudio;
    QCheckBox *labFile;
    QCheckBox *rawText;
    QCheckBox *removeTone;
};


#endif // DATASET_TOOLS_EXPORTDIALOG_H
