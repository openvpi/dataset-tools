#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

namespace dstools {

class ExportSettingsPanel : public QWidget {
    Q_OBJECT

public:
    explicit ExportSettingsPanel(QWidget *parent = nullptr);
    ~ExportSettingsPanel() override = default;

    QWidget *createDisplayTab();
    QWidget *createGeneralTab();
    QWidget *createPreprocessTab();

    void collectAndSaveAutoSave();
    void collectAndSaveChartLayout();
    void collectAndSaveLanguage();
    void collectAndSaveDefaultResolution();

    void connectDirtySignals();

signals:
    void dirtyChanged();

private:
    QComboBox *m_languageCombo = nullptr;
    QCheckBox *m_autoSaveEnabled = nullptr;
    QSpinBox *m_autoSaveInterval = nullptr;

    QListWidget *m_chartOrderList = nullptr;
    QPushButton *m_chartUpBtn = nullptr;
    QPushButton *m_chartDownBtn = nullptr;

    QSpinBox *m_defaultResolutionSpin = nullptr;
};

} // namespace dstools