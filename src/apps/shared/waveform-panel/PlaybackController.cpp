#include "PlaybackController.h"

namespace dstools {
namespace waveform {

PlaybackController::PlaybackController(QObject *parent)
    : QObject(parent), m_playWidget(new dstools::widgets::PlayWidget()) {
    // PlayWidget is hidden — only used as a playback backend
    m_playWidget->hide();

    connect(m_playWidget, &dstools::widgets::PlayWidget::playheadChanged, this,
            &PlaybackController::playheadChanged);
}

PlaybackController::~PlaybackController() {
    delete m_playWidget;
}

void PlaybackController::openFile(const QString &path) {
    m_playWidget->openFile(path);
}

void PlaybackController::playSegment(double startSec, double endSec) {
    m_playWidget->setPlayRange(startSec, endSec);
    m_playWidget->seek(startSec);
    m_playWidget->setPlaying(true);
}

void PlaybackController::stop() {
    m_playWidget->setPlaying(false);
}

bool PlaybackController::isPlaying() const {
    return m_playWidget->isPlaying();
}

} // namespace waveform
} // namespace dstools
