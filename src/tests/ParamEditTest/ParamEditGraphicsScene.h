//
// Created by fluty on 2023/9/19.
//

#ifndef DATASET_TOOLS_PARAMEDITGRAPHICSSCENE_H
#define DATASET_TOOLS_PARAMEDITGRAPHICSSCENE_H

#include <QGraphicsScene>

#include "ParamModel.h"


class ParamEditGraphicsScene : QGraphicsScene {
public:
    void loadParam(const ParamModel::RealParamCurve &param);
    void saveParam(ParamModel::RealParamCurve &param);

protected:

};



#endif // DATASET_TOOLS_PARAMEDITGRAPHICSSCENE_H
