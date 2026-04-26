#include "NoteTierLabel.h"
#include <dstools/DsPitchDocument.h>
#include <dstools/PitchUtils.h>

#include <QPainter>
#include <QFontMetrics>

namespace dstools {
namespace pitchlabeler {
namespace ui {

NoteTierLabel::NoteTierLabel(QWidget *parent)
    : TierLabelArea(parent)
{
    setMinimumHeight(24);
    setMaximumHeight(24);
}

void NoteTierLabel::setDsPitchDocument(std::shared_ptr<DsPitchDocument> file)
{
    m_file = std::move(file);
    update();
}

void NoteTierLabel::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fill background
    painter.fillRect(rect(), QColor(0x2A, 0x2A, 0x36));
    
    if (!m_file || m_file->notes.empty() || !m_viewport) {
        // Draw placeholder text
        painter.setPen(QColor(0x98, 0x98, 0xA8));
        painter.drawText(rect(), Qt::AlignCenter, "No notes");
        return;
    }

    // Get viewport range
    double viewStart = m_viewport->startSec();
    double viewEnd = m_viewport->endSec();
    double viewWidth = viewEnd - viewStart;
    if (viewWidth <= 0) {
        return;
    }

    // Setup font and colors
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    
    QColor textColor(0x98, 0x98, 0xA8);
    QColor separatorColor(0x4A, 0x90, 0xD9);
    painter.setPen(textColor);

    // Draw each note in the visible range
    const auto &notes = m_file->notes;
    int visibleNoteCount = 0;
    
    for (size_t i = 0; i < notes.size(); ++i) {
        const auto &note = notes[i];
        double noteStart = note.start / 1e6; // microseconds to seconds
        double noteEnd = note.end() / 1e6;
        
        // Check if note is visible
        if (noteEnd < viewStart || noteStart > viewEnd) {
            continue;
        }

        // Calculate x position
        double noteCenter = (noteStart + noteEnd) / 2.0;
        double normalizedX = (noteCenter - viewStart) / viewWidth;
        int x = static_cast<int>(normalizedX * width());

        // Prepare text: note number + pitch name
        QString text;
        if (note.isRest()) {
            text = QString("%1 R").arg(i + 1);
        } else {
            // Convert note name to pitch name
            auto pitch = parseNoteName(note.name);
            QString pitchName;
            if (pitch.valid) {
                pitchName = QString("%1%2").arg(pitch.name).arg(pitch.octave);
            } else {
                pitchName = note.name;
            }
            text = QString("%1 %2").arg(i + 1).arg(pitchName);
        }

        // Draw text
        QFontMetrics metrics(font);
        int textWidth = metrics.horizontalAdvance(text);
        int textX = x - textWidth / 2;
        
        // Ensure text stays within bounds
        if (textX < 0) textX = 0;
        if (textX + textWidth > width()) textX = width() - textWidth;
        
        painter.drawText(textX, 16, text);

        // Draw separator between notes (except after last visible note)
        if (i + 1 < notes.size()) {
            const auto &nextNote = notes[i + 1];
            double nextStart = nextNote.start / 1e6;
            if (nextStart >= viewStart && nextStart <= viewEnd) {
                double separatorX = (nextStart - viewStart) / viewWidth * width();
                painter.setPen(separatorColor);
                painter.drawLine(static_cast<int>(separatorX), 4, static_cast<int>(separatorX), height() - 4);
                painter.setPen(textColor);
            }
        }

        visibleNoteCount++;
    }

    // If no notes visible, show placeholder
    if (visibleNoteCount == 0) {
        painter.setPen(textColor);
        painter.drawText(rect(), Qt::AlignCenter, "No notes in view");
    }
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools