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

    class RealParam {
    public:
        int offset;// tick
        QList<float> values;
    };

    // Real Params Data
    RealParam realBreathiness;
    RealParam realEnergy;

    static ParamModel load(const QJsonObject &obj, bool *ok = nullptr);
};



#endif // DATASET_TOOLS_PARAMMODEL_H
