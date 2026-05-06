#include <dsfw/LogPanelWidget.h>
#include <dsfw/LogNotifier.h>
#include <dsfw/widgets/LogViewer.h>

#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTextStream>
#include <QVBoxLayout>

namespace dsfw {

// ── Mapping dstools::LogLevel → LogViewer::LogLevel ─────────────────

static widgets::LogViewer::LogLevel mapLevel(dstools::LogLevel lvl) {
    switch (lvl) {
        case dstools::LogLevel::Trace:   return widgets::LogViewer::Debug;
        case dstools::LogLevel::Debug:   return widgets::LogViewer::Debug;
        case dstools::LogLevel::Info:    return widgets::LogViewer::Info;
        case dstools::LogLevel::Warning: return widgets::LogViewer::Warning;
        case dstools::LogLevel::Error:   return widgets::LogViewer::Error;
        case dstools::LogLevel::Fatal:   return widgets::LogViewer::Error;
    }
    return widgets::LogViewer::Debug;
}

// ── Construction ─────────────────────────────────────────────────────

LogPanelWidget::LogPanelWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Title bar
    auto *titleBar = new QHBoxLayout();
    auto *title = new QLabel(tr("Log"), this);
    title->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 12px;"));
    titleBar->addWidget(title);
    titleBar->addStretch();
    layout->addLayout(titleBar);

    // Viewer
    m_viewer = new widgets::LogViewer(this);
    layout->addWidget(m_viewer, 1);

    // Bottom toolbar
    auto *toolbar = new QHBoxLayout();

    toolbar->addWidget(new QLabel(tr("Category:"), this));

    m_categoryFilter = new QComboBox(this);
    m_categoryFilter->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_categoryFilter->addItem(tr("All"), QString());
    connect(m_categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogPanelWidget::applyCategoryFilter);
    toolbar->addWidget(m_categoryFilter);

    toolbar->addStretch();

    m_clearButton = new QPushButton(tr("Clear"), this);
    connect(m_clearButton, &QPushButton::clicked, this, &LogPanelWidget::clear);
    toolbar->addWidget(m_clearButton);

    m_exportButton = new QPushButton(tr("Export..."), this);
    connect(m_exportButton, &QPushButton::clicked, this, &LogPanelWidget::exportToFile);
    toolbar->addWidget(m_exportButton);

    layout->addLayout(toolbar);
}

// ── Public API ───────────────────────────────────────────────────────

void LogPanelWidget::connectToNotifier() {
    // Prepopulate from ring buffer
    auto entries = dstools::Logger::instance().recentEntries(500);
    for (const auto &entry : entries)
        appendEntry(entry);

    // Live updates
    connect(&dstools::LogNotifier::instance(), &dstools::LogNotifier::entryAdded,
            this, &LogPanelWidget::appendEntry);
}

void LogPanelWidget::appendEntry(const dstools::LogEntry &entry) {
    // Track category
    QString cat = QString::fromStdString(entry.category);
    if (!m_knownCategories.contains(cat)) {
        m_knownCategories.insert(cat);
        rebuildCategoryFilter();
    }

    // Apply category filter
    if (!m_activeCategories.isEmpty() && !m_activeCategories.contains(cat))
        return;

    // Format: "[HH:mm:ss] category: message"
    auto time = QDateTime::fromMSecsSinceEpoch(entry.timestampMs);
    QString msg = QStringLiteral("%1  %2: %3")
                      .arg(time.toString("HH:mm:ss.zzz"),
                           cat,
                           QString::fromStdString(entry.message));

    m_viewer->appendLog(mapLevel(entry.level), msg);
}

void LogPanelWidget::exportToFile() {
    QString path = QFileDialog::getSaveFileName(
        this, tr("Export Log"), QString(), tr("Log files (*.log);;All files (*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Cannot write to file: %1").arg(path));
        return;
    }

    QTextStream stream(&file);
    auto entries = dstools::Logger::instance().recentEntries(500);
    for (const auto &entry : entries)
        stream << QString::fromStdString(entry.toString()) << "\n";

    DSFW_LOG_INFO("ui", ("Log exported to " + path.toStdString()).c_str());
}

void LogPanelWidget::clear() {
    m_viewer->clear();
    dstools::Logger::instance().clearRingBuffer();
}

// ── Private ──────────────────────────────────────────────────────────

void LogPanelWidget::rebuildCategoryFilter() {
    QString current = m_categoryFilter->currentData().toString();
    m_categoryFilter->blockSignals(true);
    m_categoryFilter->clear();
    m_categoryFilter->addItem(tr("All"), QString());

    for (const auto &cat : m_knownCategories) {
        m_categoryFilter->addItem(cat, cat);
        if (cat == current)
            m_categoryFilter->setCurrentIndex(m_categoryFilter->count() - 1);
    }
    m_categoryFilter->blockSignals(false);
}

void LogPanelWidget::applyCategoryFilter() {
    QString cat = m_categoryFilter->currentData().toString();
    m_activeCategories.clear();
    if (!cat.isEmpty())
        m_activeCategories.insert(cat);

    // Rebuild display with current filter
    m_viewer->clear();
    auto entries = dstools::Logger::instance().recentEntries(500);
    for (const auto &entry : entries) {
        QString ecat = QString::fromStdString(entry.category);
        if (!m_activeCategories.isEmpty() && !m_activeCategories.contains(ecat))
            continue;
        auto time = QDateTime::fromMSecsSinceEpoch(entry.timestampMs);
        QString msg = QStringLiteral("%1  %2: %3")
                          .arg(time.toString("HH:mm:ss.zzz"),
                               ecat,
                               QString::fromStdString(entry.message));
        m_viewer->appendLog(mapLevel(entry.level), msg);
    }
}

} // namespace dsfw
