#include "AudioSettingsPanel.h"
#include "Keys.h"

#include <dsfw/AppSettings.h>

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>
#include <SDL2/SDL.h>

namespace dstools {

using namespace dsfw;

AudioSettingsPanel::AudioSettingsPanel(QWidget *parent) : QWidget(parent) {}

QWidget *AudioSettingsPanel::createAudioTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    auto *group = new QGroupBox(QStringLiteral("音频输出设备"), w);
    auto *formLayout = new QFormLayout(group);

    m_audioDeviceCombo = new QComboBox(group);
    m_audioDeviceCombo->setToolTip(QStringLiteral("选择音频输出设备。切换后下次播放生效。"));

    m_audioDeviceCombo->blockSignals(true);
    m_audioDeviceCombo->addItem(QStringLiteral("(系统默认)"), QString());

    if (SDL_Init(SDL_INIT_AUDIO) == 0) {
        int count = SDL_GetNumAudioDevices(0);
        for (int i = 0; i < count; ++i) {
            const char *name = SDL_GetAudioDeviceName(i, 0);
            if (name) {
                m_audioDeviceCombo->addItem(QString::fromUtf8(name), QString::fromUtf8(name));
            }
        }
    } else {
        m_audioDeviceCombo->addItem(QStringLiteral("(无可用音频设备)"), QString());
        m_audioDeviceCombo->setEnabled(false);
    }

    AppSettings settings(QStringLiteral("DsLabeler"));
    QString savedDevice = settings.get(settings::app::kAudioDevice);
    if (!savedDevice.isEmpty()) {
        for (int i = 0; i < m_audioDeviceCombo->count(); ++i) {
            if (m_audioDeviceCombo->itemData(i).toString() == savedDevice) {
                m_audioDeviceCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    m_audioDeviceCombo->blockSignals(false);

    formLayout->addRow(QStringLiteral("输出设备:"), m_audioDeviceCombo);

    layout->addWidget(group);

    auto *infoLabel = new QLabel(
        QStringLiteral("选择音频输出设备。\n"
                       "如果某些区域播放不出声，尝试切换设备。\n"
                       "设备切换将在下次播放时生效。"),
        w);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet(QStringLiteral("color: gray; font-style: italic; margin-top: 8px;"));
    layout->addWidget(infoLabel);

    layout->addStretch();
    return w;
}

void AudioSettingsPanel::connectDirtySignals() {
    connect(m_audioDeviceCombo, &QComboBox::currentIndexChanged, this, [this]() {
        AppSettings settings(QStringLiteral("DsLabeler"));
        settings.set(settings::app::kAudioDevice,
                     m_audioDeviceCombo->currentData().toString());
        emit dirtyChanged();
    });
}

} // namespace dstools