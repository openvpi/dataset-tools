//
// Created by fluty on 2024/1/27.
//

#ifndef DSPXMODEL_H
#define DSPXMODEL_H

#include "DsTrack.h"

class DsModel final : public QObject {
    Q_OBJECT

public:
    enum ChangeType { Insert, Update, Remove};
    int numerator = 4;
    int denominator = 4;

    double tempo() const;
    void setTempo(double tempo);
    QList<DsTrack> tracks() const;
    void insertTrack(const DsTrack &track, int index);
    void removeTrack(int index);

    bool loadAProject(const QString &filename);

public slots:
    void onTrackUpdated(int index);
    void onSelectedClipChanged(int trackIndex, int clipIndex);

signals:
    void modelChanged(const DsModel &model);
    void tempoChanged(double tempo);
    void tracksChanged(ChangeType type, const DsModel &model , int index);
    void selectedClipChanged(const DsModel &model, int trackIndex, int clipIndex);

private:
    void reset();
    void runG2p();

    double m_tempo = 120;
    QList<DsTrack> m_tracks;

    // instance
    int m_selectedClipTrackIndex = -1;
    int m_selectedClipIndex = -1;
};



#endif // DSPXMODEL_H
