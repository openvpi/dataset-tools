#include "ChartCoordinator.h"
#include "IBoundaryModel.h"
#include "BoundaryDragController.h"

#include <dstools/TimePos.h>

#include <algorithm>

namespace dstools {
namespace phonemelabeler {

ChartCoordinator::ChartCoordinator(QObject *parent)
    : QObject(parent)
{
}

void ChartCoordinator::addPanel(IChartPanel *panel) {
    if (!panel) return;
    m_panels.push_back(panel);
    m_panelMap[panel->chartId()] = panel;
}

void ChartCoordinator::removePanel(const QString &chartId) {
    auto it = m_panelMap.find(chartId);
    if (it != m_panelMap.end()) {
        auto pit = std::find(m_panels.begin(), m_panels.end(), it->second);
        if (pit != m_panels.end())
            m_panels.erase(pit);
        m_panelMap.erase(it);
    }
}

IChartPanel *ChartCoordinator::panel(const QString &chartId) {
    auto it = m_panelMap.find(chartId);
    return (it != m_panelMap.end()) ? it->second : nullptr;
}

std::vector<QString> ChartCoordinator::chartIds() const {
    std::vector<QString> ids;
    for (const auto *p : m_panels)
        ids.push_back(p->chartId());
    return ids;
}

void ChartCoordinator::onViewportStateUpdate(const ViewportState &state, int pixelWidth) {
    m_xf.updateFromState(state, pixelWidth);

    for (auto *panel : m_panels)
        panel->onViewportUpdate(m_xf);

    emit viewportChanged(m_xf);
}

void ChartCoordinator::onPlayheadUpdate(const PlayheadState &state) {
    m_playhead = state;

    for (auto *panel : m_panels)
        panel->onPlayheadUpdate(m_playhead);

    emit playheadChanged(m_playhead);
}

void ChartCoordinator::onActiveTierChanged(int tierIndex) {
    for (auto *panel : m_panels)
        panel->onActiveTierChanged(tierIndex);
}

void ChartCoordinator::setBoundaryModel(IBoundaryModel *model) {
    m_boundaryModel = model;

    for (auto *panel : m_panels)
        panel->setBoundaryModel(model);
}

void ChartCoordinator::notifyAll() {
    for (auto *panel : m_panels) {
        panel->onViewportUpdate(m_xf);
        panel->onPlayheadUpdate(m_playhead);
    }
}

void ChartCoordinator::setDragController(BoundaryDragController *ctrl) {
    m_dragController = ctrl;

    for (auto *panel : m_panels)
        panel->setDragController(ctrl);
}

void ChartCoordinator::setPlayWidget(dstools::widgets::PlayWidget *pw) {
    m_playWidget = pw;

    for (auto *panel : m_panels)
        panel->setPlayWidget(pw);
}

void ChartCoordinator::setUndoStack(QUndoStack *stack) {
    m_undoStack = stack;

    for (auto *panel : m_panels)
        panel->setUndoStack(stack);
}

} // namespace phonemelabeler
} // namespace dstools