#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QVector>

namespace dstools::labeler {

class CleanupDialog : public QDialog {
    Q_OBJECT
public:
    explicit CleanupDialog(const QString &workingDir, QWidget *parent = nullptr);

    QVector<int> selectedSteps() const; // 0-based step indices

private:
    QString m_workingDir;
    QVector<QCheckBox *> m_checkBoxes;

    void buildLayout();
    void scanDirectory();
    void selectAll();
    void deselectAll();
};

} // namespace dstools::labeler
