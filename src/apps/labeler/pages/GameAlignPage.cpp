#include "GameAlignPage.h"

#include <QDir>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <game-infer/Game.h>

namespace dstools::labeler {

GameAlignPage::GameAlignPage(QWidget *parent) : QWidget(parent) {
    buildUi();
}

void GameAlignPage::buildUi() {
    auto *vLayout = new QVBoxLayout(this);

    // Parameter panel
    auto *form = new QFormLayout;

    m_modelPath = new dstools::widgets::PathSelector(dstools::widgets::PathSelector::Directory, "",
                                                     "");
    m_modelPath->setPlaceholder(tr("Path to GAME model directory"));
    form->addRow(tr("Model:"), m_modelPath);

    m_gpuSelector = new dstools::widgets::GpuSelector;
    form->addRow(tr("Device:"), m_gpuSelector);

    vLayout->addLayout(form);

    // Run progress row
    m_runProgress = new dstools::widgets::RunProgressRow(tr("Run"));
    vLayout->addWidget(m_runProgress);

    // Log area
    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    vLayout->addWidget(m_log, 1);

    // Run action
    m_runAction = new QAction(tr("Run MIDI Alignment"), this);
    m_runAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    auto runSlot = [this]() {
        if (m_workingDir.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("No working directory set."));
            return;
        }
        if (m_modelPath->path().isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("No GAME model directory selected."));
            return;
        }

        const QString csvPath = m_workingDir + QStringLiteral("/dstemp/build_csv/transcriptions.csv");
        const QString outCsvPath = m_workingDir + QStringLiteral("/dstemp/midi/transcriptions.csv");
        QDir().mkpath(m_workingDir + QStringLiteral("/dstemp/midi"));

        m_log->clear();
        m_log->append(tr("Loading GAME model..."));

        Game::Game game;
        auto [ep, deviceId] = m_gpuSelector->selected();
        std::string msg;
        auto provider = static_cast<Game::ExecutionProvider>(static_cast<int>(ep));
        if (!game.load_model(m_modelPath->path().toStdString(), provider, deviceId, msg)) {
            m_log->append(tr("<b>Error:</b> Failed to load model: %1").arg(QString::fromStdString(msg)));
            return;
        }

        m_log->append(tr("Running GAME alignment on CSV..."));
        m_runProgress->setProgress(0);

        Game::AlignOptions alignOpts;
        if (!game.alignCSV(
                csvPath.toStdString(), outCsvPath.toStdString(), "", false, alignOpts, msg,
                [this](int progress) { m_runProgress->setProgress(progress); })) {
            m_log->append(tr("<b>Error:</b> %1").arg(QString::fromStdString(msg)));
            QMessageBox::warning(this, tr("MIDI Alignment Failed"), QString::fromStdString(msg));
        } else {
            m_runProgress->setProgress(100);
            m_log->append(tr("<b>Done.</b> Output: %1").arg(outCsvPath));
        }
    };
    connect(m_runProgress, &dstools::widgets::RunProgressRow::runClicked, this, runSlot);
    connect(m_runAction, &QAction::triggered, this, runSlot);
}

QList<QAction *> GameAlignPage::editActions() const {
    return {m_runAction};
}

void GameAlignPage::setWorkingDirectory(const QString &dir) {
    m_workingDir = dir;
}

QString GameAlignPage::workingDirectory() const {
    return m_workingDir;
}

int GameAlignPage::progressTotal() const {
    return m_progressTotal;
}

int GameAlignPage::progressCurrent() const {
    return m_progressCurrent;
}

bool GameAlignPage::isRunning() const {
    return m_running;
}

QString GameAlignPage::progressMessage() const {
    return m_progressMessage;
}

void GameAlignPage::cancelOperation() {
    m_running = false;
}

} // namespace dstools::labeler
