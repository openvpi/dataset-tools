#include "StepNavigator.h"

#include <QFont>

namespace dstools::labeler {

static const char *stepNames[] = {
    "1. Slice", "2. ASR",   "3. Label", "4. Align", "5. Phone",
    "6. CSV",   "7. MIDI",  "8. DS",    "9. Pitch",
};

StepNavigator::StepNavigator(QWidget *parent) : QListWidget(parent) {
    setFixedWidth(180);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto f = font();
    f.setPointSize(11);
    setFont(f);

    setSpacing(2);
    setIconSize(QSize(0, 0));

    buildItems();

    connect(this, &QListWidget::currentRowChanged, this, &StepNavigator::stepSelected);
}

void StepNavigator::buildItems() {
    for (int i = 0; i < StepCount; ++i) {
        auto *item = new QListWidgetItem(this);
        m_statuses[i] = NotStarted;
        updateItemIcon(i);
        item->setSizeHint(QSize(0, 36));
    }
}

void StepNavigator::setStepStatus(int step, StepStatus status) {
    if (step < 0 || step >= StepCount)
        return;
    m_statuses[step] = status;
    updateItemIcon(step);
}

StepNavigator::StepStatus StepNavigator::stepStatus(int step) const {
    if (step < 0 || step >= StepCount)
        return NotStarted;
    return m_statuses[step];
}

void StepNavigator::updateItemIcon(int step) {
    if (step < 0 || step >= StepCount)
        return;

    const char *prefix;
    switch (m_statuses[step]) {
        case InProgress: prefix = "\u25C9 "; break; // ◉
        case Completed:  prefix = "\u2713 "; break;  // ✓
        case HasError:   prefix = "\u2715 "; break;   // ✕
        default:         prefix = "\u25CB "; break;    // ○
    }

    item(step)->setText(QString::fromUtf8(prefix) + QString::fromUtf8(stepNames[step]));
}

} // namespace dstools::labeler
