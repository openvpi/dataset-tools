//
// Created by fluty on 2024/1/27.
//

#ifndef DSPARAMS_H
#define DSPARAMS_H

#include <QVector>

#include "DsCurve.h"

class DsParam {
public:
    QVector<DsCurve> original;
    QVector<DsCurve> edited;
    QVector<DsCurve> envelope;
};

class DsParams {
public:
    DsParam pitch;
    DsParam energy;
    DsParam tension;
    DsParam breathiness;
};



#endif //DSPARAMS_H
