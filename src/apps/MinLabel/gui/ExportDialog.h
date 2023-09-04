#ifndef DATASET_TOOLS_EXPORTDIALOG_H
#define DATASET_TOOLS_EXPORTDIALOG_H


#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>


class ExportDialog : public QDialog {
    Q_OBJECT
public:
    explicit ExportDialog(QWidget *parent = nullptr);
    ~ExportDialog() override;

    QString dirPath;
    QString outputDir;
    bool convertPinyin;

    QLineEdit *dirnameEdit;
    QPushButton *dirnameButton;

    QLineEdit *outputDirEdit;
    QCheckBox *convertFilename;
};


#endif // DATASET_TOOLS_EXPORTDIALOG_H
