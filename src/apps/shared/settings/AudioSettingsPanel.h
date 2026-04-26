#pragma once

#include <QComboBox>
#include <QWidget>

namespace dstools {

class AudioSettingsPanel : public QWidget {
    Q_OBJECT

public:
    explicit AudioSettingsPanel(QWidget *parent = nullptr);
    ~AudioSettingsPanel() override = default;

    QWidget *createAudioTab();

    void connectDirtySignals();

signals:
    void dirtyChanged();

private:
    QComboBox *m_audioDeviceCombo = nullptr;
};

} // namespace dstools