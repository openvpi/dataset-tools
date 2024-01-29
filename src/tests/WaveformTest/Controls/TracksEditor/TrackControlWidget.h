//
// Created by fluty on 2024/1/29.
//

#ifndef TRACKCONTROLWIDGET_H
#define TRACKCONTROLWIDGET_H

#include <QHBoxLayout>
#include <QPushButton>
#include <QString>

#include "../Base/EditLabel.h"
// #include "../Base/LevelMeter.h"
#include "../Base/SeekBar.h"
#include "Model/DsTrack.h"
#include "Model/DsTrackControl.h"



class TrackControlWidget final : public QWidget {
    Q_OBJECT

public:
    explicit TrackControlWidget(QWidget *parent = nullptr);
    int trackIndex() const;
    void setTrackIndex(int i);
    QString name() const;
    void setName(const QString &name);
    DsTrackControl control() const;
    void setControl(const DsTrackControl &control);

signals:
    void propertyChanged();
    // void propertyChanged(const QString &name, const QString &value);

public slots:
    void onTrackUpdated(const DsTrack &track);

private slots:
    void onSeekBarValueChanged();

private:
    // controls
    QPushButton *m_btnColor;
    QLabel *m_lbTrackIndex;
    QPushButton *m_btnMute;
    QPushButton *m_btnSolo;
    EditLabel *m_leTrackName;
    SeekBar *m_sbarPan;
    EditLabel *m_lePan;
    SeekBar *m_sbarGain;
    EditLabel *m_leVolume;
    QSpacerItem *m_panVolumeSpacer;
    // LevelMeter *m_levelMeter;
    QHBoxLayout *m_mainLayout;
    QVBoxLayout *m_controlWidgetLayout;
    QHBoxLayout *m_muteSoloTrackNameLayout;
    QHBoxLayout *m_panVolumeLayout;

    int m_buttonSize = 24;
};



#endif // TRACKCONTROLWIDGET_H
