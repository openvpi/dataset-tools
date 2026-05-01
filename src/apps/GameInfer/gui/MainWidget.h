#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>
#include <QWidget>

#include <dsfw/AppSettings.h>
#include <dsfw/ITranscriptionService.h>
#include <dstools/GpuSelector.h>
#include <dstools/PathSelector.h>
#include <dstools/RunProgressRow.h>

#include <QFuture>
#include <map>
#include <string>

class MainWidget : public QWidget {
    Q_OBJECT

public:
    explicit MainWidget(dstools::AppSettings *settings, QWidget *parent = nullptr);
    ~MainWidget() override;

private slots:
    bool loadModel(const QString &modelPathText, int gpuIndex, std::string &message);
    void resetToDefaults() const;
    void onWavPathChanged(const QString &wavPath) const;
    void generateMidiOutputPath(const QString &wavPath) const;
    void onExportMidiTask();
    void onAlignCsvTask();

private:
    void setupModelGroup();
    void setupAudioGroup();
    void setupAlignGroup();
    void setupActionButtons();
    void setupProcessingGroup();
    void updateParameterValues() const;
    void loadLanguagesFromConfig(const QString &modelPath);
    void updateLanguageCombo();
    void updateTimeStepInfo(const QString &modelPath);
    void setModelLoadingStatus(const QString &status);

    dstools::widgets::PathSelector *m_modelPath;
    QComboBox *m_providerCombo;
    dstools::widgets::GpuSelector *m_deviceCombo;
    QLabel *m_modelStatusLabel;

    QDoubleSpinBox *m_segThresholdSpin;
    QSpinBox *m_segRadiusFrameSpin;
    QLabel *m_segRadiusMsLabel;

    QDoubleSpinBox *m_estThresholdSpin;

    QComboBox *m_segD3PMNStepsCombo;

    QComboBox *m_languageCombo;
    QDoubleSpinBox *m_tempoSpin;

    dstools::widgets::PathSelector *m_wavPath;
    dstools::widgets::PathSelector *m_outputMidi;
    dstools::widgets::RunProgressRow *m_audioRun;

    dstools::widgets::PathSelector *m_alignCsvInput;
    dstools::widgets::PathSelector *m_alignWavDir;
    dstools::widgets::PathSelector *m_alignOutput;
    dstools::widgets::RunProgressRow *m_alignRun;

    QPushButton *m_resetParamsBtn;

    dstools::AppSettings *m_settings;

    dstools::ITranscriptionService *m_service;

    std::map<int, std::string> m_languageIdToName;
    std::map<std::string, int> m_languageNameToId;

    int max_audio_seg_length = 60;

    float m_timeStepSeconds;
    double m_framesPerSecond;

    QFuture<void> m_runningTask;
};

#endif // MAINWIDGET_H
