#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QSpinBox>
#include <QWidget>

#include <dstools/AppSettings.h>
#include <dstools/GpuSelector.h>
#include <game-infer/Game.h>
#include <nlohmann/json.hpp>

#include <QFuture>
#include <map>
#include <string>

class MainWidget : public QWidget {
    Q_OBJECT

public:
    explicit MainWidget(dstools::AppSettings *settings, QWidget *parent = nullptr);
    ~MainWidget() override;

private slots:
    void browseModelPath();
    bool loadModel(std::string &message);
    void resetToDefaults() const;
    void onBrowseWavPath();
    void onBrowseOutputMidi();
    void onWavPathChanged(const QString &wavPath) const;
    void generateMidiOutputPath(const QString &wavPath) const;
    void onExportMidiTask();
    void onBrowseAlignCsvInput();
    void onBrowseAlignWavDir();
    void onBrowseAlignOutput();
    void onAlignCsvTask();

private:
    void setupModelGroup();
    void setupAudioGroup();
    void setupAlignGroup();
    void setupActionButtons();
    void setupProcessingGroup();
    bool updateParameterValues() const;
    void loadLanguagesFromConfig(const std::filesystem::path &modelPath);
    void updateLanguageCombo();
    void updateTimeStepInfo(const std::filesystem::path &modelPath);
    void setModelLoadingStatus(const QString &status);

    // Model group widgets
    QLineEdit *m_modelPathEdit;
    QPushButton *m_browseModelBtn;
    QPushButton *m_loadModelBtn;
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
    QLineEdit *m_wavPathLineEdit;
    QLineEdit *m_outputMidiLineEdit;
    QPushButton *m_wavPathButton;
    QPushButton *m_outputMidiButton;
    QProgressBar *m_progressBar;
    QPushButton *m_runButton;

    // Align group widgets
    QLineEdit *m_alignCsvInputEdit;
    QLineEdit *m_alignWavDirEdit;
    QLineEdit *m_alignOutputEdit;
    QPushButton *m_alignCsvInputBtn;
    QPushButton *m_alignWavDirBtn;
    QPushButton *m_alignOutputBtn;
    QProgressBar *m_alignProgressBar;
    QPushButton *m_alignRunBtn;

    // Action buttons
    QPushButton *m_resetParamsBtn;

    // Settings
    dstools::AppSettings *m_settings;

    // Game instance
    std::shared_ptr<Game::Game> m_game = nullptr;

    // Language mapping
    std::map<int, std::string> m_languageIdToName;
    std::map<std::string, int> m_languageNameToId;

    int max_audio_seg_length = 60;

    // Time step information
    float m_timeStepSeconds;
    double m_framesPerSecond;

    QFuture<void> m_runningTask;
};

#endif // MAINWIDGET_H