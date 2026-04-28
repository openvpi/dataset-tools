#include "TierEditWidget.h"
#include "IntervalTierView.h"
#include "TextGridDocument.h"
#include "ViewportController.h"
#include "BoundaryBindingManager.h"

#include <QVBoxLayout>
#include <QScrollBar>
#include <QResizeEvent>

namespace dstools {
namespace phonemelabeler {

TierEditWidget::TierEditWidget(TextGridDocument *doc, QUndoStack *undoStack,
                               ViewportController *viewport, BoundaryBindingManager *bindingMgr,
                               QWidget *parent)
    : QWidget(parent),
    m_document(doc),
    m_undoStack(undoStack),
    m_viewport(viewport),
    m_bindingManager(bindingMgr),
    m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(2);

    if (m_viewport) {
        connect(m_viewport, &ViewportController::viewportChanged,
                this, &TierEditWidget::onViewportChanged);
        m_viewStart = m_viewport->state().startSec;
        m_viewEnd = m_viewport->state().endSec;
        m_pixelsPerSecond = m_viewport->state().pixelsPerSecond;
    }

    if (m_document) {
        rebuildTierViews();
    }
}

TierEditWidget::~TierEditWidget() = default;

void TierEditWidget::setDocument(TextGridDocument *doc) {
    m_document = doc;
    rebuildTierViews();
}

void TierEditWidget::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    m_pixelsPerSecond = state.pixelsPerSecond;

    for (auto *view : m_tierViews) {
        view->setViewport(state);
    }
}

void TierEditWidget::onViewportChanged(const ViewportState &state) {
    setViewport(state);
}

void TierEditWidget::rebuildTierViews() {
    // Clear existing views
    qDeleteAll(m_tierViews);
    m_tierViews.clear();

    if (!m_document) return;

    int count = m_document->tierCount();
    for (int i = 0; i < count; ++i) {
        if (m_document->isIntervalTier(i)) {
            auto *view = new IntervalTierView(i, m_document, m_undoStack,
                                              m_viewport, m_bindingManager, this);
            view->setViewport({m_viewStart, m_viewEnd, m_pixelsPerSecond});
            view->setActive(i == m_document->activeTierIndex());

            connect(view, &IntervalTierView::activated, this, [this](int tierIndex) {
                if (m_document) {
                    m_document->setActiveTierIndex(tierIndex);
                    // Update visual state of all tier views
                    for (auto *v : m_tierViews) {
                        v->setActive(v->tierIndex() == tierIndex);
                    }
                    emit tierActivated(tierIndex);
                }
            });

            m_layout->addWidget(view);
            m_tierViews.append(view);
        }
    }
    m_layout->addStretch();
}

void TierEditWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

} // namespace phonemelabeler
} // namespace dstools
