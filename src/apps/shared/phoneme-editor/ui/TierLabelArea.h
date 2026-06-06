#pragma once

#include "ChartCoordinate.h"

#include <QList>
#include <QWidget>
#include <dsfw/widgets/ViewportController.h>

namespace dstools {

    namespace dstools {
        class IBoundaryModel;
    }
    using dstools::IBoundaryModel;

    using dsfw::widgets::ViewportController;
    using dsfw::widgets::ViewportState;

    class TierLabelArea : public QWidget {
        Q_OBJECT

    public:
        explicit TierLabelArea(QWidget *parent = nullptr);
        ~TierLabelArea() override;

        void setBoundaryModel(IBoundaryModel *model);
        IBoundaryModel *boundaryModel() const;

        void setVisibleRange(double startSec, double endSec);
        void setViewportController(ViewportController *vp);
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

        [[nodiscard]] double timeToX(double time) const;
        [[nodiscard]] dstools::ChartCoordinate buildCoordinate() const;
    };

} // namespace dstools
