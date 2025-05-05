#ifndef BATCHWIDGET_H
#define BATCHWIDGET_H


#include <QSettings>
#include <memory>

#include <QProgressBar>

#include <some-infer/Some.h>


class QPushButton;
class QLineEdit;

class BatchWidget final : public QWidget {
    Q_OBJECT

public:
    explicit BatchWidget(std::shared_ptr<Some::Some> some, QSettings *cfg, QWidget *parent = nullptr);

    QLineEdit *m_wavPathLineEdit;
    QLineEdit *m_csvLineEdit;

private slots:
    void onBrowseWavPath();
    void onBrowseOutputCsv();

    void onExportMidiTask() const;

private:
    QSettings *m_cfg;
    std::shared_ptr<Some::Some> m_some;
    QPushButton *m_wavPathButton;
    QPushButton *m_csvButton;

    QProgressBar *m_progressBar;
    QPushButton *m_runButton;
};

#endif // BATCHWIDGET_H
