#pragma once

/// @file TierLabelArea.h
/// @brief Base class for tier label areas displayed above the chart splitter.

#include <QWidget>

namespace dstools {

namespace phonemelabeler {
class IBoundaryModel;
}
using phonemelabeler::IBoundaryModel;

class TierLabelArea : public QWidget {
    Q_OBJECT

public:
    explicit TierLabelArea(QWidget *parent = nullptr);
    ~TierLabelArea() override;

    void setBoundaryModel(IBoundaryModel *model);
    IBoundaryModel *boundaryModel() const;

    virtual int activeTierIndex() const;
    virtual void setActiveTierIndex(int index);

signals:
    void activeTierChanged(int index);

protected:
    IBoundaryModel *m_boundaryModel = nullptr;
    int m_activeTierIndex = 0;
};

} // namespace dstools
