#include <dstools/ModelLoadPanel.h>
#include <dstools/PathSelector.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace dstools::widgets {

ModelLoadPanel::ModelLoadPanel(const QString &label, const QString &filter, QWidget *parent)
    : QWidget(parent) {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    // Top row: path selector
    m_pathSelector = new PathSelector(PathSelector::OpenFile, label, filter, this);
    mainLayout->addWidget(m_pathSelector);

    // Bottom row: Load button + status
    auto *bottomRow = new QHBoxLayout;
    bottomRow->setContentsMargins(0, 0, 0, 0);

    m_loadBtn = new QPushButton(tr("Load"), this);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);

    bottomRow->addWidget(m_loadBtn);
    bottomRow->addWidget(m_statusLabel, 1);
    mainLayout->addLayout(bottomRow);

    connect(m_pathSelector, &PathSelector::pathChanged, this, &ModelLoadPanel::pathChanged);
    connect(m_loadBtn, &QPushButton::clicked, this, [this]() {
        emit loadRequested(m_pathSelector->path());
    });
}

QString ModelLoadPanel::path() const {
    return m_pathSelector->path();
}

PathSelector *ModelLoadPanel::pathSelector() const {
    return m_pathSelector;
}

void ModelLoadPanel::setStatus(const QString &text, bool success) {
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(success ? QStringLiteral("color: green;")
                                         : QStringLiteral("color: red;"));
}

void ModelLoadPanel::setLoading(bool loading) {
    m_loadBtn->setEnabled(!loading);
    if (loading) {
        m_statusLabel->setText(tr("Loading..."));
        m_statusLabel->setStyleSheet(QStringLiteral("color: gray;"));
    }
}

} // namespace dstools::widgets
