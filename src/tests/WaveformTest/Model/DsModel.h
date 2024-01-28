//
// Created by fluty on 2024/1/27.
//

#ifndef DSPXMODEL_H
#define DSPXMODEL_H

#include "DsTrack.h"

class DsModel final : public QObject {
    Q_OBJECT

public:
    int numerator = 4;
    int denominator = 4;
    double tempo = 120;

    QList<DsTrack> tracks() const;
    void addTrack(const DsTrack &track);
    // TODO: interfaces

    bool loadAProject(const QString &filename);

signals:
    void modelChanged();

private:
    void reset();
    void runG2p();

    QList<DsTrack> m_tracks;
};



#endif // DSPXMODEL_H
