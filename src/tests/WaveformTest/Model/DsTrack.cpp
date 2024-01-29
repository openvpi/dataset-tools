//
// Created by FlutyDeer on 2023/12/1.
//

#include <QDebug>

#include "DsTrack.h"

QString DsTrack::name() const {
    return m_name;
}
void DsTrack::setName(const QString &name) {
    m_name = name;
    qDebug() << "model name " << name;
}
DsTrackControl DsTrack::control() const {
    return m_control;
}
void DsTrack::setControl(const DsTrackControl &control) {
    m_control = control;
}
DsTrack::DsTrack() {
}
DsTrack::~DsTrack() {
    // qDebug() << "~DsTrack";
}