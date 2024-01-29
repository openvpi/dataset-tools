//
// Created by FlutyDeer on 2023/12/1.
//

#ifndef TRACKSMODEL_H
#define TRACKSMODEL_H

#include <QString>

#include "DsClip.h"
#include "DsTrackControl.h"

class DsTrack {
public:
    DsTrack();
    ~DsTrack();

    QString name() const;
    void setName(const QString &name);
    DsTrackControl control() const;
    void setControl(const DsTrackControl &control);
    QList<DsClipPtr> clips;
    // void addClip(DsClip clip);
    // void removeClip(const Clip &clip);
    // void removeClip(int index);

    // Color color() const;
    // void setColor(const Color &color);

private:
    QString m_name;
    DsTrackControl m_control = DsTrackControl();
    QList<DsClipPtr> m_clips;
};



#endif // TRACKSMODEL_H
