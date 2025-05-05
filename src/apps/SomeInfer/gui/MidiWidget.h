#ifndef MIDIWIDGET_H
#define MIDIWIDGET_H

#include <memory>

#include <QProgressBar>
#include <QSettings>

#include <some-infer/Some.h>


class QLineEdit;
class QPushButton;

class MidiWidget final : public QWidget {
    Q_OBJECT

public:
    explicit MidiWidget(std::shared_ptr<Some::Some> some, QSettings *cfg, QWidget *parent = nullptr);

    QLineEdit *m_wavPathLineEdit;
    QLineEdit *m_tempoLineEdit;
    QLineEdit *m_outputMidiLineEdit;

private slots:
    void onBrowseWavPath();
    void onBrowseOutputMidi();

    void onExportMidiTask() const;

private:
    QSettings *m_cfg;
    std::shared_ptr<Some::Some> m_some;
    QPushButton *m_wavPathButton;
    QPushButton *m_outputMidiButton;

    QProgressBar *m_progressBar;
    QPushButton *m_runButton;
};

#endif // MIDIWIDGET_H