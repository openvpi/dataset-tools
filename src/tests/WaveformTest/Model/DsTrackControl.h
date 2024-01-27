//
// Created by fluty on 2024/1/27.
//

#ifndef DSTRACKCONTROL_H
#define DSTRACKCONTROL_H

class DsTrackControl {
public:
    double gain() const;
    void setGain(double gain);
    double pan() const;
    void setPan(double pan);
    bool mute() const;
    void setMute(bool mute);
    bool solo() const;
    void setSolo(bool solo);

private:
    double m_gain = 1.0;
    double m_pan = 0;
    bool m_mute = false;
    bool m_solo = false;
};

#endif // DSTRACKCONTROL_H
