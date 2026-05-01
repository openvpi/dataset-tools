/// @file MainWidget.h
/// @brief GameInfer main control panel with model loading, parameter tuning, and task execution.

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>
#include <QWidget>

#include <dsfw/AppSettings.h>
#include <dsfw/ITaskProcessor.h>
#include <dstools/GpuSelector.h>
#include <dstools/PathSelector.h>
#include <dstools/RunProgressRow.h>

#include <QFuture>
#include <map>
#include <memory>
#include <string>

/// @brief Central widget containing model path selection, GPU configuration,
/// GAME parameters, and MIDI export/CSV alignment controls.
class MainWidget : public QWidget {
    Q_OBJECT

public:
    /// @param settings Application settings instance.
    /// @param parent Optional parent widget.
    explicit MainWidget(dstools::AppSettings *settings, QWidget *parent = nullptr);
    ~MainWidget() override;

private slots:
    /// @brief Load a GAME model from the given path.
    /// @param modelPathText Path to the model directory.
    /// @param gpuIndex GPU device index.
    /// @param[out] message Status or error message.
    /// @return True if the model loaded successfully.
    bool loadModel(const QString &modelPathText, int gpuIndex, std::string &message);

    /// @brief Reset all GAME parameters to their default values.
    void resetToDefaults() const;

    /// @brief Handle WAV path changes to update dependent fields.
    void onWavPathChanged(const QString &wavPath) const;

    /// @brief Auto-generate the MIDI output path from the WAV path.
    void generateMidiOutputPath(const QString &wavPath) const;

    /// @brief Run the MIDI export task.
    void onExportMidiTask();

    /// @brief Run the CSV alignment task.
    void onAlignCsvTask();

private:
    void setupModelGroup();
    void setupAudioGroup();
    void setupAlignGroup();
    void setupActionButtons();
    void setupProcessingGroup();
    void loadLanguagesFromConfig(const QString &modelPath);
    void updateLanguageCombo();
    void updateTimeStepInfo(const QString &modelPath);
    void setModelLoadingStatus(const QString &status);

    dstools::widgets::PathSelector *m_modelPath;  ///< Model directory path selector.
    QComboBox *m_providerCombo;                    ///< Inference provider selector.
    dstools::widgets::GpuSelector *m_deviceCombo;  ///< GPU device selector.
    QLabel *m_modelStatusLabel;                    ///< Model loading status display.

    QDoubleSpinBox *m_segThresholdSpin;   ///< Segmentation threshold spin box.
    QSpinBox *m_segRadiusFrameSpin;       ///< Segmentation radius (frames) spin box.
    QLabel *m_segRadiusMsLabel;           ///< Segmentation radius in milliseconds label.

    QDoubleSpinBox *m_estThresholdSpin;   ///< Estimation threshold spin box.

    QComboBox *m_segD3PMNStepsCombo;      ///< D3PM timesteps combo box.

    QComboBox *m_languageCombo;           ///< Language selection combo box.
    QDoubleSpinBox *m_tempoSpin;          ///< Tempo (BPM) spin box for MIDI export.

    dstools::widgets::PathSelector *m_wavPath;     ///< Input WAV file path selector.
    dstools::widgets::PathSelector *m_outputMidi;  ///< Output MIDI file path selector.
    dstools::widgets::RunProgressRow *m_audioRun;  ///< MIDI export progress row.

    dstools::widgets::PathSelector *m_alignCsvInput;  ///< CSV alignment input path selector.
    dstools::widgets::PathSelector *m_alignWavDir;    ///< WAV directory for alignment.
    dstools::widgets::PathSelector *m_alignOutput;    ///< Aligned CSV output path selector.
    dstools::widgets::RunProgressRow *m_alignRun;     ///< CSV alignment progress row.

    QPushButton *m_resetParamsBtn;  ///< Reset parameters button.

    dstools::AppSettings *m_settings;                        ///< Application settings.
    std::unique_ptr<dstools::ITaskProcessor> m_processor;    ///< Task processor instance.
    bool m_initialized = false;                              ///< Whether processor is initialized.

    std::map<int, std::string> m_languageIdToName; ///< Language ID to name mapping.
    std::map<std::string, int> m_languageNameToId; ///< Language name to ID mapping.

    int max_audio_seg_length = 60; ///< Maximum audio segment length in seconds.

    float m_timeStepSeconds;   ///< Duration of one model timestep in seconds.
    double m_framesPerSecond;  ///< Model frame rate.

    QFuture<void> m_runningTask; ///< Currently running async task future.
};

#endif // MAINWIDGET_H
