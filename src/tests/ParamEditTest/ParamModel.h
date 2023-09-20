//
// Created by fluty on 2023/9/5.
//

#ifndef DATASET_TOOLS_PARAMMODEL_H
#define DATASET_TOOLS_PARAMMODEL_H

#include <QJsonObject>

class ParamModel {
public:
    ParamModel();
    ~ParamModel();

    class Note {
    public:
        int start;
        int length;
        int keyIndex;
        QString lyric;
    };

    class RealParamCurve {
    public:
        int offset;// tick
        QList<float> values;
    };

    QList<Note> notes;

    // Real Params Data
    QList<RealParamCurve> realBreathiness;
    QList<RealParamCurve> realEnergy;

    static ParamModel load(const QJsonObject &array, bool *ok = nullptr);
};



#endif // DATASET_TOOLS_PARAMMODEL_H
