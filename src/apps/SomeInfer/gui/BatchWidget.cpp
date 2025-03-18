#include "BatchWidget.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <iostream>

#include <wolf-midi/MidiFile.h>
BatchWidget::BatchWidget(std::shared_ptr<Some::Some> some, SomeCfg *cfg, QWidget *parent)
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

    const auto outputCsvLayout = new QHBoxLayout();
    const auto outputMidiLabel = new QLabel("输出csv：", this);
    m_csvLineEdit = new QLineEdit(this);
    m_csvLineEdit->setText(m_cfg->outMidiPath);
    m_csvButton = new QPushButton("浏览...", this);
    outputCsvLayout->addWidget(outputMidiLabel);
    outputCsvLayout->addWidget(m_csvLineEdit);
    outputCsvLayout->addWidget(m_csvButton);

    const auto csvLabel = new QLabel(
        "注意：请自行备份原csv！！！运行程序会覆盖csv内note_seq、note_dur列！不对任何情况导致的数据丢失负责！", this);
    csvLabel->setWordWrap(true);
    mainLayout->addWidget(csvLabel);
    mainLayout->addLayout(outputCsvLayout);
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


    connect(m_wavPathButton, &QPushButton::clicked, this, &BatchWidget::onBrowseWavPath);
    connect(m_csvButton, &QPushButton::clicked, this, &BatchWidget::onBrowseOutputCsv);

    connect(m_runButton, &QPushButton::clicked, this, &BatchWidget::onExportMidiTask);
}

void BatchWidget::onBrowseWavPath() {
    const QString wavPath = QFileDialog::getOpenFileName(this, "选择输入wav文件", "", "WAV文件 (*.wav)");
    if (!wavPath.isEmpty()) {
        m_wavPathLineEdit->setText(wavPath);
        m_cfg->wavPath = wavPath;
    }
}

void BatchWidget::onBrowseOutputCsv() {
    const QString file = QFileDialog::getSaveFileName(this, "选择输出csv文件", "", "CSV文件 (*.csv)");
    if (!file.isEmpty()) {
        m_csvLineEdit->setText(file);
        m_cfg->outMidiPath = file;
    }
}

void BatchWidget::onExportMidiTask() const {
    m_runButton->setEnabled(false);

    QFuture<void> future = QtConcurrent::run([this]() {
        std::vector<Some::Midi> midis;
        std::string msg;
        m_progressBar->setValue(0);

        const bool success = m_some->get_midi(
            m_wavPathLineEdit->text().toLocal8Bit().toStdString(), midis, 5512.5, msg, [=](const int progress) {
                QMetaObject::invokeMethod(m_progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
            });

        if (success) {
        } else {
            std::cerr << "Error: " << msg << std::endl;
        }

        QMetaObject::invokeMethod(m_runButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    });
}