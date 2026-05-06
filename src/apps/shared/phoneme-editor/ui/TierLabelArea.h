#pragma once

#include <QList>
#include <QWidget>

#include <dstools/ViewportController.h>

namespace dstools {

namespace phonemelabeler {
class IBoundaryModel;
}
using phonemelabeler::IBoundaryModel;

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

class TierLabelArea : public QWidget {
    Q_OBJECT

public:
    explicit TierLabelArea(QWidget *parent = nullptr);
    ~TierLabelArea() override;

    void setBoundaryModel(IBoundaryModel *model);
    IBoundaryModel *boundaryModel() const;

    void setViewportController(ViewportController *viewport);
    ViewportController *viewportController() const;

    virtual int activeTierIndex() const;
    virtual void setActiveTierIndex(int index);

    virtual QList<double> activeBoundaries() const;

    /// Called when the underlying boundary model data has changed.
    /// Default implementation calls update(). Subclasses may override
    /// to rebuild UI (e.g. radio buttons for tier selection).
    virtual void onModelDataChanged();

signals:
    void activeTierChanged(int index);

protected:
    IBoundaryModel *m_boundaryModel = nullptr;
    ViewportController *m_viewport = nullptr;
    int m_activeTierIndex = 0;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;

    [[nodiscard]] int timeToX(double time) const;
};

} // namespace dstools
