#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QSpinBox>
#include <QWidget>

#include <game-infer/Game.h>
#include <nlohmann/json.hpp>

#include <map>
#include <string>

class MainWidget : public QWidget {
    Q_OBJECT

public:
    explicit MainWidget(QSettings *settings, QWidget *parent = nullptr);

private slots:
    void browseModelPath();
    void loadModel();
    void resetToDefaults() const;
    void onBrowseWavPath();
    void onBrowseOutputMidi();
    void onWavPathChanged(const QString &wavPath) const;
    void generateMidiOutputPath(const QString &wavPath) const;
    void onExportMidiTask();

private:
    void setupModelGroup();
    void setupAudioGroup();
    void setupActionButtons();
    void updateDeviceList() const;
    void setupProcessingGroup();
    void updateParameterValues();
    void loadLanguagesFromConfig(const std::filesystem::path &modelPath);
    void updateLanguageCombo();
    void updateTimeStepInfo(const std::filesystem::path &modelPath);
    void setModelLoadingStatus(const QString &status, bool isLoading = false);

    // Model group widgets
    QLineEdit *m_modelPathEdit;
    QPushButton *m_browseModelBtn;
    QPushButton *m_loadModelBtn;
    QComboBox *m_providerCombo;
    QComboBox *m_deviceCombo;
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

    // Action buttons
    QPushButton *m_resetParamsBtn;

    // Settings
    QSettings *m_settings;

    // Game instance
    std::shared_ptr<Game::Game> m_game;

    // Language mapping
    std::map<int, std::string> m_languageIdToName;
    std::map<std::string, int> m_languageNameToId;

    // Time step information
    float m_timeStepSeconds;
    double m_framesPerSecond;

    // Loading state
    bool m_isLoading;
};

#endif // MAINWIDGET_H