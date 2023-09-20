//
// Created by fluty on 2023/9/5.
//

#ifndef DATASET_TOOLS_PARAMEDITAREA_H
#define DATASET_TOOLS_PARAMEDITAREA_H

#include <QFrame>

#include <ParamModel.h>

class ParamEditArea : public QFrame {
    Q_OBJECT

public:
    explicit ParamEditArea(QWidget *parent = nullptr);
    ~ParamEditArea() override;

    void loadParam(const ParamModel::RealParamCurve &param);
    void saveParam(ParamModel::RealParamCurve &param);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    ParamModel::RealParamCurve m_param;
};



#endif // DATASET_TOOLS_PARAMEDITAREA_H
