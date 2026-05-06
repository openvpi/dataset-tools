#include "LogPage.h"

#include <dsfw/LogNotifier.h>
#include <dsfw/widgets/LogViewer.h>

#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QTextStream>
#include <QVBoxLayout>

namespace dstools {

static dsfw::widgets::LogViewer::LogLevel mapLevel(dstools::LogLevel lvl) {
    switch (lvl) {
        case dstools::LogLevel::Trace:   return dsfw::widgets::LogViewer::Debug;
        case dstools::LogLevel::Debug:   return dsfw::widgets::LogViewer::Debug;
        case dstools::LogLevel::Info:    return dsfw::widgets::LogViewer::Info;
        case dstools::LogLevel::Warning: return dsfw::widgets::LogViewer::Warning;
        case dstools::LogLevel::Error:   return dsfw::widgets::LogViewer::Error;
        case dstools::LogLevel::Fatal:   return dsfw::widgets::LogViewer::Error;
    }
    return dsfw::widgets::LogViewer::Debug;
}

LogPage::LogPage(QWidget *parent)
    : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_viewer = new dsfw::widgets::LogViewer(this);
    layout->addWidget(m_viewer, 1);

    auto *toolbar = new QHBoxLayout();

    toolbar->addWidget(new QLabel(tr("Category:"), this));

    m_categoryFilter = new QComboBox(this);
    m_categoryFilter->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_categoryFilter->addItem(tr("All"), QString());
    connect(m_categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogPage::applyCategoryFilter);
    toolbar->addWidget(m_categoryFilter);

    toolbar->addStretch();

    m_clearButton = new QPushButton(tr("Clear"), this);
    connect(m_clearButton, &QPushButton::clicked, this, &LogPage::clear);
    toolbar->addWidget(m_clearButton);

    m_exportButton = new QPushButton(tr("Export..."), this);
    connect(m_exportButton, &QPushButton::clicked, this, &LogPage::exportToFile);
    toolbar->addWidget(m_exportButton);

    layout->addLayout(toolbar);

    auto entries = dstools::Logger::instance().recentEntries(500);
    for (const auto &entry : entries)
        appendEntry(entry);

    connect(&dstools::LogNotifier::instance(), &dstools::LogNotifier::entryAdded,
            this, &LogPage::appendEntry);
}

QMenuBar *LogPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);

    auto *viewMenu = bar->addMenu(tr("&View"));

    auto *clearAction = viewMenu->addAction(tr("&Clear Log"));
    connect(clearAction, &QAction::triggered, this, &LogPage::clear);

    auto *exportAction = viewMenu->addAction(tr("&Export Log..."));
    connect(exportAction, &QAction::triggered, this, &LogPage::exportToFile);

    return bar;
}

QString LogPage::windowTitle() const {
    return tr("Log");
}

void LogPage::onActivated() {
    m_viewer->setFocus();
}

void LogPage::onDeactivated() {}

void LogPage::appendEntry(const dstools::LogEntry &entry) {
    QString cat = QString::fromStdString(entry.category);
    if (!m_knownCategories.contains(cat)) {
        m_knownCategories.insert(cat);
        rebuildCategoryFilter();
    }

    if (!m_activeCategories.isEmpty() && !m_activeCategories.contains(cat))
        return;

    auto time = QDateTime::fromMSecsSinceEpoch(entry.timestampMs);
    QString msg = QStringLiteral("%1  %2: %3")
                      .arg(time.toString("HH:mm:ss.zzz"),
                           cat,
                           QString::fromStdString(entry.message));

    m_viewer->appendLog(mapLevel(entry.level), msg);
}

void LogPage::rebuildCategoryFilter() {
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

void LogPage::applyCategoryFilter() {
    QString cat = m_categoryFilter->currentData().toString();
    m_activeCategories.clear();
    if (!cat.isEmpty())
        m_activeCategories.insert(cat);

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

void LogPage::exportToFile() {
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
}

void LogPage::clear() {
    m_viewer->clear();
    dstools::Logger::instance().clearRingBuffer();
}

} // namespace dstools
