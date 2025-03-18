#ifndef MIDIWIDGET_H
#define MIDIWIDGET_H

#include <QWidget>
#include <memory>

#include <QProgressBar>

#include <some-infer/Some.h>

#include "inc/SomeCfg.h"

class QLineEdit;
class QPushButton;

class MidiWidget final : public QWidget {
    Q_OBJECT

public:
    explicit MidiWidget(std::shared_ptr<Some::Some> some, SomeCfg *cfg, QWidget *parent = nullptr);

    QLineEdit *m_wavPathLineEdit;
    QLineEdit *m_tempoLineEdit;
    QLineEdit *m_outputMidiLineEdit;

private slots:
    void onBrowseWavPath();
    void onBrowseOutputMidi();

    void onExportMidiTask() const;

private:
    SomeCfg *m_cfg;
    std::shared_ptr<Some::Some> m_some;
    QPushButton *m_wavPathButton;
    QPushButton *m_outputMidiButton;

    QProgressBar *m_progressBar;
    QPushButton *m_runButton;
};

#endif // MIDIWIDGET_H