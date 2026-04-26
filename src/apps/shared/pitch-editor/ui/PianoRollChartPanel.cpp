#include "PianoRollChartPanel.h"

#include <QVBoxLayout>
#include <dsfw/AppSettings.h>
#include "PianoRollRenderer.h"

namespace dstools {
namespace pitchlabeler {
namespace ui {

PianoRollChartPanel::PianoRollChartPanel(dsfw::widgets::ViewportController *vc,
                                         QWidget *parent)
    : chart::ChartPanelBase("pianoroll", vc, parent)
    , m_pianoRoll(new PianoRollView(this))
{
    // Set up piano roll
    m_pianoRoll->setViewportController(vc);
    m_pianoRoll->setContentLeftMargin(0);
    
    // Connect signals from piano roll to our signals
    connect(m_pianoRoll, &PianoRollView::noteSelected,
            this, &PianoRollChartPanel::noteSelected);
    connect(m_pianoRoll, &PianoRollView::toolModeChanged,
            this, &PianoRollChartPanel::toolModeChanged);
    connect(m_pianoRoll, &PianoRollView::fileEdited,
            this, &PianoRollChartPanel::fileEdited);
    connect(m_pianoRoll, &PianoRollView::positionClicked,
            this, &PianoRollChartPanel::positionClicked);
    connect(m_pianoRoll, &PianoRollView::rulerClicked,
            this, &PianoRollChartPanel::rulerClicked);
    connect(m_pianoRoll, &PianoRollView::noteGlideChanged,
            this, &PianoRollChartPanel::noteGlideChanged);
    connect(m_pianoRoll, &PianoRollView::noteSlurToggled,
            this, &PianoRollChartPanel::noteSlurToggled);
    connect(m_pianoRoll, &PianoRollView::noteRestToggled,
            this, &PianoRollChartPanel::noteRestToggled);
    connect(m_pianoRoll, &PianoRollView::noteMergeLeft,
            this, &PianoRollChartPanel::noteMergeLeft);
    connect(m_pianoRoll, &PianoRollView::noteDeleteRequested,
            this, &PianoRollChartPanel::noteDeleteRequested);
    connect(m_pianoRoll, &PianoRollView::verticalScrolled,
            this, &PianoRollChartPanel::verticalContentScrolled);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_pianoRoll);
}

void PianoRollChartPanel::setDsPitchDocument(std::shared_ptr<DsPitchDocument> file)
{
    m_pianoRoll->setDsPitchDocument(file);
}

void PianoRollChartPanel::setUndoStack(QUndoStack *stack)
{
    m_pianoRoll->setUndoStack(stack);
}

void PianoRollChartPanel::setBoundaryModel(chart::IBoundaryModel *model)
{
    // Note: PianoRollView doesn't use boundary model directly
    // This is for future integration with NoteBoundaryModel
    ChartPanelBase::setBoundaryModel(model);
}

void PianoRollChartPanel::drawContent(QPainter &painter, const chart::ChartCoordinate &coord)
{
    m_pianoRoll->render(painter, coord);
}

void PianoRollChartPanel::onViewportUpdate(const chart::ChartCoordinate &conv, int pixelWidth)
{
    ChartPanelBase::onViewportUpdate(conv, pixelWidth);
}

void PianoRollChartPanel::onBoundaryModelInvalidated()
{
    ChartPanelBase::onBoundaryModelInvalidated();
}

void PianoRollChartPanel::paintYAxisContent(QPainter &painter, const QRect &rect)
{
    auto rs = m_pianoRoll->buildRenderState();
    painter.save();
    painter.translate(rect.topLeft());
    PianoRollRenderer::drawPianoKeys(painter, rect.height(), rs);
    painter.restore();
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools