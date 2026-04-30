#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>
#include <QWidget>

#include <dstools/AppSettings.h>
#include <dstools/GpuSelector.h>
#include <dstools/PathSelector.h>
#include <dstools/RunProgressRow.h>
#include <game-infer/Game.h>

#include <QFuture>
#include <map>
#include <string>

class GameInferService;

class MainWidget : public QWidget {
    Q_OBJECT

public:
    explicit MainWidget(dstools::AppSettings *settings, QWidget *parent = nullptr);
    ~MainWidget() override;

private slots:
    bool loadModel(const QString &modelPathText, Game::ExecutionProvider provider, int deviceId, std::string &message);
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
    void loadLanguagesFromConfig(const std::filesystem::path &modelPath);
    void updateLanguageCombo();
    void updateTimeStepInfo(const std::filesystem::path &modelPath);
    void setModelLoadingStatus(const QString &status);

    // Model group widgets
    dstools::widgets::PathSelector *m_modelPath;
    QComboBox *m_providerCombo;
    dstools::widgets::GpuSelector *m_deviceCombo;
    QLabel *m_modelStatusLabel;

    // Segmentation group widgets
    QDoubleSpinBox *m_segThresholdSpin;
    QSpinBox *m_segRadiusFrameSpin;
    QLabel *m_segRadiusMsLabel;

    // Estimation group widgets
    QDoubleSpinBox *m_estThresholdSpin;

    // D3PM group widgets
    QComboBox *m_segD3PMNStepsCombo;

    // Other group widgets
    QComboBox *m_languageCombo;
    QDoubleSpinBox *m_tempoSpin;

    // Audio processing widgets
    dstools::widgets::PathSelector *m_wavPath;
    dstools::widgets::PathSelector *m_outputMidi;
    dstools::widgets::RunProgressRow *m_audioRun;

    // Align group widgets
    dstools::widgets::PathSelector *m_alignCsvInput;
    dstools::widgets::PathSelector *m_alignWavDir;
    dstools::widgets::PathSelector *m_alignOutput;
    dstools::widgets::RunProgressRow *m_alignRun;

    // Action buttons
    QPushButton *m_resetParamsBtn;

    // Settings
    dstools::AppSettings *m_settings;

    // Service (owns Game instance + mutex)
    GameInferService *m_service;

    // Language mapping (UI-side, populated from config)
    std::map<int, std::string> m_languageIdToName;
    std::map<std::string, int> m_languageNameToId;

    int max_audio_seg_length = 60;

    // Time step information (UI display only)
    float m_timeStepSeconds;
    double m_framesPerSecond;

    QFuture<void> m_runningTask;
};

#endif // MAINWIDGET_H
