#include "MidiWidget.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <iostream>

#include <wolf-midi/MidiFile.h>

static void makeMidiFile(const std::filesystem::path &midi_path, std::vector<Some::Midi> midis, const float tempo) {
    Midi::MidiFile midi;
    midi.setFileFormat(1);
    midi.setDivisionType(Midi::MidiFile::DivisionType::PPQ);
    midi.setResolution(480);

    midi.createTrack();

    midi.createTimeSignatureEvent(0, 0, 4, 4);
    midi.createTempoEvent(0, 0, tempo);

    std::vector<char> trackName;
    std::string str = "Some";
    trackName.insert(trackName.end(), str.begin(), str.end());

    midi.createTrack();
    midi.createMetaEvent(1, 0, Midi::MidiEvent::MetaNumbers::TrackName, trackName);

    for (const auto &[note, start, duration] : midis) {
        midi.createNoteOnEvent(1, start, 0, note, 64);
        midi.createNoteOffEvent(1, start + duration, 0, note, 64);
    }

    midi.save(midi_path);
}


MidiWidget::MidiWidget(std::shared_ptr<Some::Some> some, SomeCfg *cfg, QWidget *parent)
    : QWidget(parent), m_cfg(cfg), m_some(std::move(some)) {
    const auto mainLayout = new QVBoxLayout(this);

    const auto wavPathLayout = new QHBoxLayout();
    const auto wavPathLabel = new QLabel("wav文件", this);
    m_wavPathLineEdit = new QLineEdit(this);
    m_wavPathLineEdit->setText(m_cfg->wavPath);
    m_wavPathButton = new QPushButton("浏览...", this);
    wavPathLayout->addWidget(wavPathLabel);
    wavPathLayout->addWidget(m_wavPathLineEdit);
    wavPathLayout->addWidget(m_wavPathButton);
    mainLayout->addLayout(wavPathLayout);

    const auto wavTip = new QLabel("注意：尽量选择单声道wav，多声道/flac/mp3仅供测试。", this);
    mainLayout->addWidget(wavTip);

    const auto tempoLayout = new QHBoxLayout();
    const auto tempoLabel = new QLabel("tempo：", this);
    m_tempoLineEdit = new QLineEdit(this);
    m_tempoLineEdit->setText(m_cfg->tempo);
    tempoLayout->addWidget(tempoLabel);
    tempoLayout->addWidget(m_tempoLineEdit);
    tempoLayout->addStretch();
    mainLayout->addLayout(tempoLayout);

    const auto outputMidiLayout = new QHBoxLayout();
    const auto outputMidiLabel = new QLabel("输出midi：", this);
    m_outputMidiLineEdit = new QLineEdit(this);
    m_outputMidiLineEdit->setText(m_cfg->outMidiPath);
    m_outputMidiButton = new QPushButton("浏览...", this);
    outputMidiLayout->addWidget(outputMidiLabel);
    outputMidiLayout->addWidget(m_outputMidiLineEdit);
    outputMidiLayout->addWidget(m_outputMidiButton);
    mainLayout->addLayout(outputMidiLayout);
    mainLayout->addStretch();

    const auto progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    progressLayout->addWidget(m_progressBar);
    m_runButton = new QPushButton("导出", this);
    progressLayout->addWidget(m_runButton);
    mainLayout->addLayout(progressLayout);


    connect(m_wavPathButton, &QPushButton::clicked, this, &MidiWidget::onBrowseWavPath);
    connect(m_outputMidiButton, &QPushButton::clicked, this, &MidiWidget::onBrowseOutputMidi);

    connect(m_runButton, &QPushButton::clicked, this, &MidiWidget::onExportMidiTask);

    connect(m_tempoLineEdit, &QLineEdit::textChanged, [this](const QString &text) { m_cfg->tempo = text; });
}

void MidiWidget::onBrowseWavPath() {
    const QString wavPath = QFileDialog::getOpenFileName(
        this, "选择输入wav文件", "",
        "音频文件 (*.wav *.flac *.mp3);;WAV文件 (*.wav);;FLAC文件 (*.flac);;MP3文件 (*.mp3)");
    if (!wavPath.isEmpty()) {
        m_wavPathLineEdit->setText(wavPath);
        m_cfg->wavPath = wavPath;
    }
}

void MidiWidget::onBrowseOutputMidi() {
    const QString file = QFileDialog::getSaveFileName(this, "选择输出midi文件", "", "MIDI文件 (*.mid)");
    if (!file.isEmpty()) {
        m_outputMidiLineEdit->setText(file);
        m_cfg->outMidiPath = file;
    }
}

void MidiWidget::onExportMidiTask() const {
    m_runButton->setEnabled(false);

    QFuture<void> future = QtConcurrent::run([this]() {
        std::vector<Some::Midi> midis;
        std::string msg;
        m_progressBar->setValue(0);

        const bool success = m_some->get_midi(m_wavPathLineEdit->text().toLocal8Bit().toStdString(), midis,
                                              m_tempoLineEdit->text().toFloat(), msg, [=](const int progress) {
                                                  QMetaObject::invokeMethod(m_progressBar, "setValue",
                                                                            Qt::QueuedConnection, Q_ARG(int, progress));
                                              });

        if (success) {
            makeMidiFile(m_outputMidiLineEdit->text().toLocal8Bit().toStdString(), midis,
                         m_tempoLineEdit->text().toFloat());
        } else {
            std::cerr << "Error: " << msg << std::endl;
        }

        QMetaObject::invokeMethod(m_runButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    });
}