#include <dsfw/widgets/PipelineStatusBar.h>
#include <dsfw/PipelineContext.h>

#include <QMouseEvent>
#include <QStyle>

namespace dsfw::widgets {

PipelineStatusBar::PipelineStatusBar(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QHBoxLayout(this)) {
    m_layout->setContentsMargins(4, 2, 4, 2);
    m_layout->setSpacing(6);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(28);
}

void PipelineStatusBar::setSteps(const QVector<StepInfo> &steps) {
    m_steps = steps;
    rebuild();
}

void PipelineStatusBar::setContext(const dsfw::PipelineContext *ctx) {
    m_ctx = ctx;
    rebuild();
}

void PipelineStatusBar::clearContext() {
    m_ctx = nullptr;
    rebuild();
}

QString PipelineStatusBar::statusChar(const StepInfo &step, const dsfw::PipelineContext *ctx) {
    if (!ctx)
        return QStringLiteral("\u25CB");

    for (const auto &rec : ctx->stepHistory) {
        if (rec.stepName == step.taskName) {
            if (rec.success)
                return QStringLiteral("\u2713");
            return QStringLiteral("\u2717");
        }
    }

    const auto completed = ctx->completedSteps();
    if (completed.contains(step.taskName))
        return QStringLiteral("\u2713");

    return QStringLiteral("\u25CB");
}

void PipelineStatusBar::rebuild() {
    for (auto *label : m_labels)
        label->deleteLater();
    m_labels.clear();

    for (const auto &step : m_steps) {
        auto *label = new QLabel(this);
        label->setFixedSize(22, 22);
        label->setAlignment(Qt::AlignCenter);

        QString style;
        QString ch = statusChar(step, m_ctx);
        label->setText(ch);

        if (ch == QStringLiteral("\u2713")) {
            style = QStringLiteral("QLabel { color: #4CAF50; font-weight: bold; font-size: 13px; }");
        } else if (ch == QStringLiteral("\u2717")) {
            style = QStringLiteral("QLabel { color: #F44336; font-weight: bold; font-size: 13px; }");
        } else {
            style = QStringLiteral("QLabel { color: #9E9E9E; font-size: 13px; }");
        }

        if (step.isManual)
            style.replace(QStringLiteral("}"), QStringLiteral(" border: 1px solid #FF9800; border-radius: 11px; }"));

        label->setStyleSheet(style);
        label->setToolTip(step.displayName);

        label->installEventFilter(this);

        m_labels.append(label);
        m_layout->addWidget(label);
    }

    m_layout->addStretch();
}

bool PipelineStatusBar::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        for (int i = 0; i < m_labels.size(); ++i) {
            if (m_labels[i] == obj && i < m_steps.size()) {
                emit stepClicked(m_steps[i].taskName);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

} // namespace dsfw::widgets