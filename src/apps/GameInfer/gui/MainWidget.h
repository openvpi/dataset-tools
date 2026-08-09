#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointF>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>

#include "InferenceQueueController.h"

#include <game-infer/Game.h>
#include <nlohmann/json.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

class QEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

class MainWidget : public QWidget {
    Q_OBJECT

public:
    explicit MainWidget(QSettings *settings, QWidget *parent = nullptr);
    ~MainWidget() override;

private slots:
    void browseModelPath();
    void resetToDefaults() const;
    void addBatchJobs();
    void applyQueueDefaultsToEditableJobs();
    void removeSelectedQueueJob();
    void moveSelectedQueueJobUp();
    void moveSelectedQueueJobDown();
    void startQueue();
    void stopQueueAfterCurrent();
    void refreshQueueTable();
    void onQueueRunningChanged(bool running);

protected:
    void changeEvent(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    enum class ModelStatus {
        NotLoaded,
        ConfigurationChanged,
        PathMissing,
        Loading,
        Loaded,
        LoadFailed,
    };

    struct ModelSelection {
        std::filesystem::path path;
        Game::ExecutionProvider provider = Game::ExecutionProvider::CPU;
        int deviceId = -1;

        bool matches(const ModelSelection &other) const {
            return path == other.path && provider == other.provider && deviceId == other.deviceId;
        }
    };

    struct ProcessingParameters {
        float segThreshold = 0.2f;
        float segRadiusFrames = 2.0f;
        float estThreshold = 0.2f;
        std::vector<float> d3pmTs;
    };

    void setupModelGroup();
    void setupActionButtons();
    void setupQueueGroup();
    void setupExecutionBar();
    void updateDeviceList() const;
    void setupProcessingGroup();
    ModelSelection currentModelSelection() const;
    ProcessingParameters currentProcessingParameters() const;
    bool loadModel(const ModelSelection &selection, std::string &message);
    bool updateParameterValues(const ProcessingParameters &parameters) const;
    void setControlsEnabled(bool enabled) const;
    void loadLanguagesFromConfig(const std::filesystem::path &modelPath);
    void updateLanguageCombo();
    void updateTimeStepInfo(const std::filesystem::path &modelPath);
    void setModelLoadingStatus(ModelStatus status);
    void retranslateUi();
    [[nodiscard]] QVector<QPair<int, QString>> availableLanguages() const;
    [[nodiscard]] quint64 selectedQueueJobId() const;
    [[nodiscard]] bool isQueueDropPosition(const QPointF &position) const;
    void addBatchJobsFromFiles(const QStringList &files);
    void updateQueueJobFromRow(int row);
    [[nodiscard]] bool validateQueueBeforeStart();

    // Model group widgets
    QGroupBox *m_modelGroup = nullptr;
    QLabel *m_modelPathLabel = nullptr;
    QLineEdit *m_modelPathEdit;
    QPushButton *m_browseModelBtn;
    QLabel *m_providerLabel = nullptr;
    QComboBox *m_providerCombo;
    QLabel *m_deviceLabel = nullptr;
    QComboBox *m_deviceCombo;
    QLabel *m_modelStatusLabel;
    ModelStatus m_modelStatus = ModelStatus::NotLoaded;

    // Segmentation group widgets
    QGroupBox *m_processingGroup = nullptr;
    QLabel *m_segThresholdLabel = nullptr;
    QDoubleSpinBox *m_segThresholdSpin;
    QLabel *m_segRadiusLabel = nullptr;
    QSpinBox *m_segRadiusFrameSpin;
    QLabel *m_segRadiusMsLabel;

    // Estimation group widgets
    QLabel *m_estThresholdLabel = nullptr;
    QDoubleSpinBox *m_estThresholdSpin;

    // D3PM group widgets
    QLabel *m_segD3PMNStepsLabel = nullptr;
    QComboBox *m_segD3PMNStepsCombo;

    // Queue default widgets
    QLabel *m_queueDefaultsLabel = nullptr;
    QLabel *m_languageLabel = nullptr;
    QComboBox *m_languageCombo;
    QLabel *m_tempoLabel = nullptr;
    QDoubleSpinBox *m_tempoSpin;
    QPushButton *m_applyQueueDefaultsButton = nullptr;

    // Action buttons
    QPushButton *m_resetParamsBtn;
    QPushButton *m_toggleQueueBtn;

    // Queue widgets
    QGroupBox *m_queueGroup;
    QTableWidget *m_queueTable;
    QPushButton *m_batchAddButton;
    QPushButton *m_removeQueueButton;
    QPushButton *m_moveQueueUpButton;
    QPushButton *m_moveQueueDownButton;
    QPushButton *m_retryFailedButton;
    QPushButton *m_startQueueButton;
    QPushButton *m_stopQueueButton;
    QLabel *m_currentQueueJobLabel;
    QLabel *m_queueSummaryLabel;
    QProgressBar *m_progressBar;
    quint64 m_currentQueueJobId = 0;
    bool m_refreshingQueueTable = false;

    // Settings
    QSettings *m_settings;

    // Game instance
    std::shared_ptr<Game::Game> m_game = nullptr;
    std::optional<ModelSelection> m_loadedModelSelection;
    InferenceQueueController *m_queueController = nullptr;

    // Language mapping
    std::map<int, std::string> m_languageIdToName;
    std::map<std::string, int> m_languageNameToId;

    int max_audio_seg_length = 60;

    // Time step information
    float m_timeStepSeconds;
    double m_framesPerSecond;
};

#endif // MAINWIDGET_H
