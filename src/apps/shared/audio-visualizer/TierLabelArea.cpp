#include "TierLabelArea.h"

#include <ui/IBoundaryModel.h>

namespace dstools {

TierLabelArea::TierLabelArea(QWidget *parent)
    : QWidget(parent) {
    setFixedHeight(24);
}

TierLabelArea::~TierLabelArea() = default;

void TierLabelArea::setBoundaryModel(IBoundaryModel *model) {
    m_boundaryModel = model;
    update();
}

IBoundaryModel *TierLabelArea::boundaryModel() const {
    return m_boundaryModel;
}

int TierLabelArea::activeTierIndex() const {
    return m_activeTierIndex;
}

void TierLabelArea::setActiveTierIndex(int index) {
    if (m_activeTierIndex != index) {
        m_activeTierIndex = index;
        emit activeTierChanged(index);
        update();
    }
}

} // namespace dstools
