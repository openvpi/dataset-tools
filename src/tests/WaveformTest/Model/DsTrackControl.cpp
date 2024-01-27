//
// Created by fluty on 2024/1/27.
//

#include "DsTrackControl.h"

double DsTrackControl::gain() const {
    return m_gain;
}
void DsTrackControl::setGain(double gain) {
    m_gain = gain;
}
double DsTrackControl::pan() const {
    return m_pan;
}
void DsTrackControl::setPan(double pan) {
    m_pan = pan;
}
bool DsTrackControl::mute() const {
    return m_mute;
}
void DsTrackControl::setMute(bool mute) {
    m_mute = mute;
}
bool DsTrackControl::solo() const {
    return m_solo;
}
void DsTrackControl::setSolo(bool solo) {
    m_solo = solo;
}