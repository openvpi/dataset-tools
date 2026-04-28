#pragma once

#include <QWidget>
#include <QVector>
#include <QPoint>
#include <QUndoStack>

#include <dstools/ViewportController.h>
#include "TextGridDocument.h"
#include "BoundaryBindingManager.h"

class QVBoxLayout;

namespace dstools {
namespace phonemelabeler {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

class IntervalTierView;

class TierEditWidget : public QWidget {
    Q_OBJECT

public:
    TierEditWidget(TextGridDocument *doc, QUndoStack *undoStack,
                   ViewportController *viewport, BoundaryBindingManager *bindingMgr,
                   QWidget *parent = nullptr);
    ~TierEditWidget() override;

    void setDocument(TextGridDocument *doc);
    void setViewport(const ViewportState &state);

public slots:
    void onViewportChanged(const ViewportState &state);

signals:
    void tierActivated(int tierIndex);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildTierViews();
    void updateViewport();

    TextGridDocument *m_document = nullptr;
    QUndoStack *m_undoStack = nullptr;
    ViewportController *m_viewport = nullptr;
    BoundaryBindingManager *m_bindingManager = nullptr;

    QVBoxLayout *m_layout = nullptr;
    QVector<IntervalTierView *> m_tierViews;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_pixelsPerSecond = 200.0;
};

} // namespace phonemelabeler
} // namespace dstools
