
#include <QApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>


#include <cmath>


#include "DsSentence.h"
#include "F0Widget.h"
#include "qjsonstream.h"


F0Widget::F0Widget(QWidget *parent) : QFrame(parent), draggingNoteInterval(0, 0), contextMenuNoteInterval(0, 0, {}) {
    hasError = false;
    assert(FrequencyToMidiNote(440.0) == 69.0);
    assert(FrequencyToMidiNote(110.0) == 45.0);

    pitchRange = {60, 60};

    setMouseTracking(true);

    horizontalScrollBar = new QScrollBar(Qt::Horizontal, this);
    verticalScrollBar = new QScrollBar(Qt::Vertical, this);
    verticalScrollBar->setRange(60000, 60000);
    verticalScrollBar->setInvertedControls(true);
    verticalScrollBar->setInvertedAppearance(true);

    noteMenu = new QMenu(this);
    noteMenuMergeLeft = new QAction("&Merge to left", noteMenu);
    noteMenuToggleRest = new QAction("&Toggle Rest", noteMenu);
    noteMenu->addAction(noteMenuMergeLeft);
    noteMenu->addAction(noteMenuToggleRest);

    bgMenu = new QMenu(this);
    bgMenuReloadSentence = new QAction("&Discard changes and reload current sentence");
    bgMenu_CautionPrompt = new QAction("Use the following with caution");
    bgMenuConvRestsToNormal = new QAction("Convert all Rests to normal notes");
    bgMenu_OptionPrompt = new QAction("Options");
    bgMenuSnapByDefault = new QAction("&Snap to piano keys by default");
    bgMenuShowPitchTextOverlay = new QAction("Show pitch text &overlay");
    auto boldMenuItemFont = bgMenu_OptionPrompt->font();
    boldMenuItemFont.setBold(true);
    bgMenu_OptionPrompt->setFont(boldMenuItemFont);
    bgMenu_OptionPrompt->setDisabled(true);
    bgMenu_CautionPrompt->setFont(boldMenuItemFont);
    bgMenu_CautionPrompt->setDisabled(true);
    bgMenuShowPitchTextOverlay->setCheckable(true);
    bgMenuSnapByDefault->setCheckable(true);
    bgMenu->addAction(bgMenuReloadSentence);
    bgMenu->addSeparator();
    bgMenu->addAction(bgMenu_CautionPrompt);
    bgMenu->addAction(bgMenuConvRestsToNormal);
    bgMenu->addSeparator();
    bgMenu->addAction(bgMenu_OptionPrompt);
    bgMenu->addAction(bgMenuSnapByDefault);
    bgMenu->addAction(bgMenuShowPitchTextOverlay);

    // Connect signals
    connect(horizontalScrollBar, &QScrollBar::valueChanged, this, &F0Widget::onHorizontalScroll);
    connect(verticalScrollBar, &QScrollBar::valueChanged, this, &F0Widget::onVerticalScroll);

    connect(noteMenuMergeLeft, &QAction::triggered, this, &F0Widget::mergeCurrentSlurToLeftNode);
    connect(noteMenuToggleRest, &QAction::triggered, this, &F0Widget::toggleCurrentNoteRest);

    connect(bgMenuReloadSentence, &QAction::triggered, this, &F0Widget::requestReloadSentence);
    connect(bgMenuConvRestsToNormal, &QAction::triggered, this, &F0Widget::convertAllRestsToNormal);
    connect(bgMenuShowPitchTextOverlay, &QAction::triggered, [=]() {
        showPitchTextOverlay = bgMenuShowPitchTextOverlay->isChecked();
        update();
    });
    connect(bgMenuSnapByDefault, &QAction::triggered, [=]() {
        snapToKey = bgMenuSnapByDefault->isChecked();
        update();
    });
}

F0Widget::~F0Widget() {
}

void F0Widget::setDsSentenceContent(const QJsonObject &content) {
    hasError = false;
    clear();

    DsSentence sentence;
    QAS::JsonStream stream(content);
    stream >> sentence;

    auto f0Seq = sentence.f0_seq.split(" ", QString::SkipEmptyParts);
    foreach (auto f0, f0Seq) {
        f0Values.append(FrequencyToMidiNote(f0.toDouble()));
    }
    f0Timestep = sentence.f0_timestep.toDouble();

    auto phSeq = sentence.ph_seq.split(" ", QString::SkipEmptyParts);
    auto phDur = sentence.ph_dur.split(" ", QString::SkipEmptyParts);
    auto phNum = sentence.ph_num.split(" ", QString::SkipEmptyParts);
    auto noteSeq = sentence.note_seq.split(" ", QString::SkipEmptyParts);
    auto noteDur = sentence.note_dur.split(" ", QString::SkipEmptyParts);
    auto text = sentence.text.split(" ", QString::SkipEmptyParts);
    auto slur = sentence.note_slur.split(" ", QString::SkipEmptyParts);

    QVector<bool> isRest(noteSeq.size(), false);

    // Sanity check
    if (noteDur.size() != noteSeq.size()) {
        setErrorStatusText(QString("Invalid DS file! Inconsistent element count\n"
                                   "note_dur: %1\n"
                                   "note_seq: %2\n"
                                   "slur(optional): %3")
                               .arg(noteDur.size())
                               .arg(noteSeq.size())
                               .arg(slur.size()));
        return;
    }

    // note_slur is optional. When there's not enough elements in it, consider it invalid and clear it.
    if (slur.size() < noteDur.size()) {
        slur.clear();
    }

#if 1
    // Give trailing rests in note_seq a valid pitch
    for (int i = noteSeq.size() - 1; i >= 0 && noteSeq[i] == "rest"; i--) {
        if (noteSeq[i] == "rest") {
            isRest[i] = true;
            for (int j = i - 1; j >= 0; j--) {
                if (noteSeq[j] != "rest") {
                    noteSeq[i] = noteSeq[j];
                    break;
                }
            }
        }
    }

    // Search from beginning of note_seq and give rests a valid pitch after it
    for (int i = 0; i < noteSeq.size(); i++) {
        if (noteSeq[i] == "rest") {
            isRest[i] = true;
            for (int j = i + 1; j < noteSeq.size(); j++) {
                if (noteSeq[j] != "rest") {
                    noteSeq[i] = noteSeq[j];
                    break;
                }
            }
        }
    }
#endif

    auto noteBegin = 0.0;
    for (int i = 0; i < noteSeq.size(); i++) {
        MiniNote note;
        note.duration = noteDur[i].toDouble();
        // Parse note pitch
        auto notePitch = noteSeq[i];
        if (notePitch.contains(QRegularExpression(R"((\+|\-))"))) {
            auto splitPitch = notePitch.split(QRegularExpression(R"((\+|\-))"));
            note.pitch = NoteNameToMidiNote(splitPitch[0]);
            note.cents = splitPitch[1].toDouble();
            note.cents *= (notePitch[splitPitch[0].length()] == '+' ? 1 : -1);
        } else {
            note.pitch = NoteNameToMidiNote(noteSeq[i]);
            note.cents = NAN;
        }
        note.isSlur = slur.empty() ? 0 : slur[i].toInt();
        note.isRest = isRest[i];
        midiIntervals.insert({noteBegin, noteBegin + note.duration, note});
        noteBegin += note.duration;
    }

    // Update ranges
    std::get<1>(timeRange) = noteBegin;
    std::get<0>(pitchRange) = *std::min_element(f0Values.begin(), f0Values.end());
    std::get<1>(pitchRange) = *std::max_element(f0Values.begin(), f0Values.end());
    horizontalScrollBar->setMaximum(noteBegin * 1000);
    verticalScrollBar->setMaximum(std::get<1>(pitchRange) * 100);
    verticalScrollBar->setMinimum(std::get<0>(pitchRange) * 100);

    isEmpty = false;

    setF0CenterAndSyncScrollbar(f0Values.first());

    update();
}

void F0Widget::setErrorStatusText(const QString &text) {
    hasError = true;
    errorStatusText = text;
    update();
}

void F0Widget::loadConfig(const SlurCutterCfg &cfg) {
    bgMenuShowPitchTextOverlay->setChecked(cfg.showPitchTextOverlay);
    bgMenuSnapByDefault->setChecked(cfg.snapToKeys);

    snapToKey = cfg.snapToKeys;
    showPitchTextOverlay = cfg.showPitchTextOverlay;

    update();
}

void F0Widget::pullConfig(SlurCutterCfg &cfg) const {
    cfg.showPitchTextOverlay = bgMenuShowPitchTextOverlay->isChecked();
    cfg.snapToKeys = bgMenuSnapByDefault->isChecked();
}

void F0Widget::clear() {
    hasError = false;
    errorStatusText = "";
    midiIntervals.clear();
    f0Values.clear();
    isEmpty = true;
    // phonemeIntervals.clear();
    // textIntervals.clear();
    update();
}

F0Widget::ReturnedDsString F0Widget::getSavedDsStrings() {
    ReturnedDsString ret;
    for (auto &i : midiIntervals.intervals()) {
        ret.note_dur += QString::number(i.value.duration, 'g', 3) + ' ';
        ret.note_slur += i.value.isSlur ? "1 " : "0 ";
        ret.note_seq += i.value.isRest
                            ? "rest "
                            : (std::isnan(i.value.cents)
                                   ? (MidiNoteToNoteName(i.value.pitch) + ' ')
                                   : (PitchToNotePlusCentsString(i.value.pitch + 0.01 * i.value.cents) + ' '));
    }
    ret.note_dur = ret.note_dur.trimmed();
    ret.note_seq = ret.note_seq.trimmed();
    ret.note_slur = ret.note_slur.trimmed();
    return ret;
};

bool F0Widget::empty() {
    return isEmpty;
}

void F0Widget::setPlayheadPos(double time) {
    qDebug() << time;
    playheadPos = time;
    auto w = width() - KeyWidth - ScrollBarWidth;
    double leftTime = centerTime - w / 2 / secondWidth, rightTime = centerTime + w / 2 / secondWidth;
    if (time > rightTime || time < leftTime)
        setTimeAxisCenterAndSyncScrollbar(time + (rightTime - leftTime) / 2);
    update();
}

int F0Widget::NoteNameToMidiNote(const QString &noteName) {
    if (noteName == "rest")
        return 0;
    static const QMap<QString, int> noteNameToMidiNote = {
        {"C",  1 },
        {"C#", 2 },
        {"D",  3 },
        {"D#", 4 },
        {"E",  5 },
        {"F",  6 },
        {"F#", 7 },
        {"G",  8 },
        {"G#", 9 },
        {"A",  10},
        {"A#", 11},
        {"B",  12},
    };
    int octave = noteName.right(1).toInt();
    return 11 + 12 * octave + noteNameToMidiNote[noteName.left(noteName.size() - 1)];
}

QString F0Widget::MidiNoteToNoteName(int note) {
    // A0 = MIDI 21, C8 = 108. Return empty when out of range
    static const char *NoteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    if (note > 108 || note < 21)
        return QString();
    note -= 12;
    return QString("%1%2").arg(NoteNames[note % 12]).arg(note / 12);
}

double F0Widget::FrequencyToMidiNote(double freq) {
    return 69 + 12 * log2(freq / 440);
}

std::tuple<int, double> F0Widget::PitchToNoteAndCents(double pitch) {
    if (pitch == 0)
        return {0, 0};
    int note = std::round(pitch);
    double cents = (pitch - note) * 100;
    return {note, cents};
}

QString F0Widget::PitchToNotePlusCentsString(double pitch) {
    if (pitch == 0)
        return "rest";
    int note = std::round(pitch);
    int cents = std::round((pitch - note) * 100);
    return QString("%1%2%3").arg(MidiNoteToNoteName(note)).arg(cents >= 0 ? "+" : "").arg(cents);
}

std::tuple<size_t, size_t> F0Widget::refF0IndexRange(double startTime, double endTime) const {
    return {std::min((size_t)::floor(std::max(0.0, startTime / f0Timestep)), (size_t) f0Values.size() - 1),
            std::min((size_t)::ceil(endTime / f0Timestep), (size_t) f0Values.size() - 1)};
}


bool F0Widget::mouseOnNote(const QPoint &mousePos, MiniNoteInterval *returnNote) const {
    auto w = width() - KeyWidth - ScrollBarWidth, h = height() - TimelineHeight;
    auto mouseTime = timeOnWidgetX(mousePos.x());
    auto mousePitch = pitchOnWidgetY(mousePos.y());

    auto matchedNotes = midiIntervals.findOverlappingIntervals({mouseTime, mouseTime}, true);
    if (matchedNotes.empty())
        return false;

    for (auto &i : matchedNotes) {
        double pitch = i.value.pitch + (std::isnan(i.value.cents) ? 0 : i.value.cents / 100);
        if (mousePitch >= pitch - 0.5 && mousePitch <= pitch + 0.5) {
            if (returnNote)
                *returnNote = i;
            return true;
        }
    }
    return false;
}

double F0Widget::pitchOnWidgetY(int y) const {
    auto h = height() - TimelineHeight;
    return centerPitch + h / 2 / semitoneHeight - (y - TimelineHeight) / semitoneHeight;
}

double F0Widget::timeOnWidgetX(int x) const {
    auto w = width() - KeyWidth - ScrollBarWidth;
    return centerTime - w / 2 / secondWidth + (x - KeyWidth) / secondWidth;
}

void F0Widget::setNoteContextMenuEntriesEnabled() {
    auto noteInterval = contextMenuNoteInterval;
    // auto leftIntervals = midiIntervals.findOverlappingIntervals({noteInterval.low - 0.1, noteInterval.low}, false);
    auto rightIntervals = midiIntervals.findOverlappingIntervals({noteInterval.high, noteInterval.high + 0.1}, false);

    // Only slurs can be merged to left
    noteMenuMergeLeft->setEnabled(noteInterval.value.isSlur);
}

void F0Widget::splitNoteUnderMouse() {
    MiniNoteInterval noteInterval{-1, -1};
    auto cursorPos = mapFromGlobal(QCursor::pos());

    if (mouseOnNote(cursorPos, &noteInterval) && !noteInterval.value.isRest) {
        auto time = timeOnWidgetX(cursorPos.x());

        MiniNote leftNote{noteInterval.value}, rightNote{noteInterval.value};
        leftNote.duration = time - noteInterval.low;
        rightNote.duration = noteInterval.high - time;
        rightNote.isSlur = true;

        midiIntervals.remove(noteInterval);
        midiIntervals.insert({noteInterval.low, time, leftNote});
        midiIntervals.insert({time, noteInterval.high, rightNote});

        update();
    }
}

void F0Widget::shiftDraggedNoteByPitch(double pitchDelta) {
    auto intervals =
        midiIntervals.findInnerIntervals({std::get<0>(draggingNoteInterval), std::get<1>(draggingNoteInterval)});
    if (intervals.empty())
        return;

    auto &note = intervals[0].value;
    double addedCents = draggingNoteInCents ? (std::isnan(note.cents) ? 0 : note.cents) : 0;
    double semitoneDelta, newCents;
    newCents = 100 * ::modf(pitchDelta + 0.01 * addedCents, &semitoneDelta);

    note.cents = newCents;
    note.pitch += semitoneDelta;

    midiIntervals.remove(intervals[0]);
    midiIntervals.insert(intervals[0]);
}

void F0Widget::setDraggedNotePitch(int pitch) {
    auto intervals =
        midiIntervals.findInnerIntervals({std::get<0>(draggingNoteInterval), std::get<1>(draggingNoteInterval)});
    if (intervals.empty())
        return;

    auto &note = intervals[0].value;
    note.pitch = pitch;
    note.cents = NAN;

    midiIntervals.remove(intervals[0]);
    midiIntervals.insert(intervals[0]);
}

void F0Widget::convertAllRestsToNormal() {
    for (auto &i : midiIntervals.intervals()) {
        if (i.value.isRest) {
            midiIntervals.remove(i);
            i.value.isRest = false;
            midiIntervals.insert(i);
        }
    }
    update();
}

void F0Widget::mergeCurrentSlurToLeftNode(bool checked) {
    Q_UNUSED(checked);

    auto noteInterval = contextMenuNoteInterval;

    auto intervals = midiIntervals.findOverlappingIntervals({noteInterval.low - 0.1, noteInterval.low}, false);
    if (intervals.empty())
        return;
    auto leftNode = intervals.back();

    midiIntervals.remove(leftNode);
    midiIntervals.remove(noteInterval);

    leftNode.value.duration += noteInterval.value.duration;
    leftNode.high = noteInterval.high;

    midiIntervals.insert(leftNode);
    update();
};

void F0Widget::toggleCurrentNoteRest() {
    auto noteInterval = contextMenuNoteInterval;

    midiIntervals.remove(noteInterval);
    noteInterval.value.isRest = !noteInterval.value.isRest;
    midiIntervals.insert(noteInterval);
    update();
}

void F0Widget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    do {
        // Fill dark grey background
        painter.fillRect(rect(), QColor(40, 40, 40));

        // Convenience references
        auto w = width(), h = height();
        const Qt::GlobalColor keyColor[] = {Qt::white, Qt::black, Qt::white, Qt::black, Qt::white, Qt::white,
                                            Qt::black, Qt::white, Qt::black, Qt::white, Qt::black, Qt::white};

        // Print centered red error string if the error flag is set
        if (hasError) {
            painter.setPen(Qt::red);
            painter.drawText(rect(), Qt::AlignCenter, errorStatusText);
            break;
        }

        // Draw time axis and marker axis
        QLinearGradient grad(0, 0, 0, TimeAxisHeight);
        grad.setColorAt(0, QColor(40, 40, 40));
        grad.setColorAt(1, QColor(60, 60, 60));
        painter.fillRect(0, HorizontalScrollHeight, w, TimeAxisHeight, grad);

        // Draw piano roll. Find the lowest drawn key from the center pitch
        painter.translate(KeyWidth, TimelineHeight);
        h = height() - TimelineHeight;
        w = width() - KeyWidth - ScrollBarWidth;
        painter.setClipRect(0, 0, w, h);

        painter.setPen(Qt::black);
        double keyReferenceY = h / 2 - (1 - ::fmod(centerPitch, 1)) * semitoneHeight; // Top of center pitch's key
        int lowestPitch = ::floor(centerPitch) - ::ceil((h - keyReferenceY) / semitoneHeight);
        double lowestPitchY = keyReferenceY + (int(centerPitch - lowestPitch) + 0.5) * semitoneHeight;

        // Grid
        painter.setPen(QColor(80, 80, 80));
        {
            auto y = lowestPitchY;
            while (y > 0) {
                painter.drawLine(0, y, w, y);
                y -= semitoneHeight;
            }
        }

        // Midi notes
        auto leftTime = centerTime - w / 2 / secondWidth, rightTime = centerTime + w / 2 / secondWidth;
        QVector<QPair<QPointF, QString>> deviatePitches;
        static constexpr QColor NoteColors[] = {QColor(106, 164, 234), QColor(60, 113, 219)};
        for (auto &i : midiIntervals.findOverlappingIntervals({leftTime, rightTime}, false)) {
            if (i.value.pitch == 0)
                continue; // Skip rests (pitch 0)
            auto rec = QRectF(
                (i.low - leftTime) * secondWidth,
                lowestPitchY - (i.value.pitch + (std::isnan(i.value.cents) ? 0 : i.value.cents) * 0.01 - lowestPitch) *
                                   semitoneHeight,
                i.value.duration * secondWidth, semitoneHeight);
            if (rec.bottom() < 0 || rec.top() > h)
                continue;
            if (!i.value.isRest) {
                painter.setPen(Qt::black);
                painter.fillRect(rec, NoteColors[i.value.isSlur]);
                painter.drawRect(rec);
                if (!std::isnan(i.value.cents)) {
                    painter.drawLine(rec.left(), rec.center().y(), rec.right(), rec.center().y());
                    // Defer the drawing of deviation text to prevent the right side notes overlapping with them
                    deviatePitches.append(
                        {rec.topLeft() + QPointF(0, -3),
                         PitchToNotePlusCentsString(i.value.pitch +
                                                    0.01 * (std::isnan(i.value.cents) ? 0 : i.value.cents))});
                }
            } else {
                auto pen = painter.pen();
                pen.setStyle(Qt::DashLine);
                pen.setColor(NoteColors[0]);
                painter.setPen(pen);
                auto brush = painter.brush();
                auto fillColor = NoteColors[i.value.isSlur];
                fillColor.setAlphaF(0.35);
                brush.setColor(fillColor);
                brush.setStyle(Qt::SolidPattern);
                painter.setBrush(brush);
                painter.drawRect(rec);
                painter.setBrush(Qt::NoBrush);
            }
            // rec.adjust(NotePadding, NotePadding, -NotePadding, -NotePadding);
            // painter.drawText(rec, Qt::AlignVCenter | Qt::AlignLeft, i.value.text);
        }

        // Drag box / hover box (Do not coexist)
        MiniNoteInterval note{-1, -1};
        if (dragging && draggingMode == Note) {
            auto pos = mapFromGlobal(QCursor::pos());
            auto mousePitch = pitchOnWidgetY(pos.y());
            auto pen = painter.pen();
            pen.setColor(Qt::white);
            pen.setStyle(Qt::SolidLine);
            pen.setWidth(0);
            painter.setPen(pen);
            auto noteLeft = (std::get<0>(draggingNoteInterval) - leftTime) * secondWidth,
                 noteRight = (std::get<1>(draggingNoteInterval) - leftTime) * secondWidth;
            painter.drawLine(noteLeft, 0, noteLeft, h);
            painter.drawLine(noteRight, 0, noteRight, h);
            if (draggingNoteInCents) {
                auto dragPitch =
                    mousePitch - draggingNoteStartPitch + draggingNoteBeginCents * 0.01 + draggingNoteBeginPitch;
                auto rec = QRectF(noteLeft, lowestPitchY - (dragPitch - lowestPitch) * semitoneHeight,
                                  noteRight - noteLeft, semitoneHeight);
                painter.fillRect(rec, QColor(255, 255, 255, 60));
                painter.drawLine(rec.left(), rec.center().y(), rec.right(), rec.center().y());
                painter.drawRect(rec);
                painter.drawText(rec.topLeft() + QPointF(0, -3), PitchToNotePlusCentsString(dragPitch));
            } else {
                auto dragPitch = ::floor(mousePitch + 0.5); // Key center pitch -> key bottom pitch
                auto rec = QRectF(noteLeft, lowestPitchY - (dragPitch - lowestPitch) * semitoneHeight,
                                  noteRight - noteLeft, semitoneHeight);
                painter.fillRect(rec, QColor(255, 255, 255, 60));
                painter.drawRect(rec);
            }
        } else if (mouseOnNote(mapFromGlobal(QCursor::pos()), &note)) {
            auto rec =
                QRectF((note.low - leftTime) * secondWidth,
                       lowestPitchY - (note.value.pitch + (std::isnan(note.value.cents) ? 0 : note.value.cents) * 0.01 -
                                       lowestPitch) *
                                          semitoneHeight,
                       note.value.duration * secondWidth, semitoneHeight);
            auto pen = painter.pen();
            pen.setColor(QColor(255, 255, 255, 128));
            pen.setStyle(Qt::SolidLine);
            pen.setWidth(5);
            painter.setPen(pen);
            painter.drawRect(rec);
        }

        // F0 Curve
        if (!f0Values.empty()) {
            // lowestPitchY is the lowest (drawn) key's Top-left y coordinate
            // But F0 curve is drawn from the center of the key, so we need to adjust it
            lowestPitchY += semitoneHeight / 2;

            auto visibleF0 = refF0IndexRange(leftTime, rightTime);
            QPainterPath path;
            double f0X = (std::get<0>(visibleF0) - leftTime / f0Timestep) * f0Timestep * secondWidth,
                   f0Ybase = lowestPitchY + lowestPitch * semitoneHeight;
            path.moveTo(f0X, lowestPitchY - (f0Values[std::get<0>(visibleF0)] - lowestPitch) * semitoneHeight);
            for (int i = std::get<0>(visibleF0) + 1; i < std::get<1>(visibleF0); i++) {
                f0X += f0Timestep * secondWidth;
                path.lineTo(f0X, lowestPitchY - (f0Values[i] - lowestPitch) * semitoneHeight);
            }
            painter.setPen(Qt::red);
            painter.drawPath(path);

            lowestPitchY -= semitoneHeight / 2;
        }

        // Deviate pitch (Note+-Cents) text
        painter.setPen(Qt::white);
        foreach (auto &i, deviatePitches) {
            painter.drawText(i.first, i.second);
        }

        painter.translate(-KeyWidth, 0);
        painter.setClipRect(0, 0, width(), h);
        w += KeyWidth;

        // Debug text
        if (showPitchTextOverlay) {
            auto mousePos = mapFromGlobal(QCursor::pos());
            auto mousePitch = centerPitch + h / 2 / semitoneHeight - (mousePos.y() - TimelineHeight) / semitoneHeight;
            painter.setPen(Qt::white);
            painter.drawText(KeyWidth, TimelineHeight,
                             QString("CenterPitch %1 (%2)  LowestPitch %3 (%4) MousePitch %5 MousePos (%6, %7)")
                                 .arg(centerPitch)
                                 .arg(MidiNoteToNoteName(centerPitch))
                                 .arg(lowestPitch)
                                 .arg(MidiNoteToNoteName(lowestPitch))
                                 .arg(mousePitch)
                                 .arg(mousePos.x())
                                 .arg(mousePos.y()));
        }

        // Piano keys
        auto prevfont = painter.font();

        do {
            lowestPitch++;
            lowestPitchY -= semitoneHeight;

            if (lowestPitch < 21)
                continue;

            auto rec = QRectF(0, lowestPitchY, 60.0, semitoneHeight);
            auto color = keyColor[lowestPitch % 12];
            painter.fillRect(rec, color);
            painter.setPen(color == Qt::white ? Qt::black : Qt::white);
            if (lowestPitch % 12 == 0) {
                auto font = prevfont;
                font.setBold(true);
                painter.setFont(font);
            } else {
                painter.setFont(prevfont);
            }
            painter.drawText(rec, Qt::AlignRight | Qt::AlignVCenter, MidiNoteToNoteName(lowestPitch));
        } while (lowestPitchY > 0 && lowestPitch <= 108);
        painter.setFont(prevfont);

        painter.translate(0, -TimelineHeight);
        painter.setClipRect(0, 0, w, height());
        h += TimelineHeight;

        // Playhead (playheadPos)
        painter.setPen(QColor(255, 180, 0));
        painter.drawLine((playheadPos - leftTime) * secondWidth + KeyWidth, TimelineHeight,
                         (playheadPos - leftTime) * secondWidth + KeyWidth, h);
    } while (false);

    painter.end();

    QFrame::paintEvent(event);
}

void F0Widget::resizeEvent(QResizeEvent *event) {
    // Update the scrollbars
    auto w = width(), h = height();
    horizontalScrollBar->setGeometry(0, 0, w, HorizontalScrollHeight);
    horizontalScrollBar->setPageStep(w / secondWidth * 1000);
    verticalScrollBar->setGeometry(w - ScrollBarWidth, TimelineHeight, ScrollBarWidth, h - TimelineHeight);
    verticalScrollBar->setPageStep((h - TimelineHeight) / semitoneHeight * 100);

    QFrame::resizeEvent(event);
}

void F0Widget::wheelEvent(QWheelEvent *event) {
    constexpr double ScrollFactorX = 10, ScrollFactorY = 2, WheelFactor = 0.1;
    auto xDelta = event->angleDelta().x(), yDelta = event->angleDelta().y();
    bool isMouseWheel = (xDelta == 0 && yDelta % 120 == 0);

    if (isMouseWheel) {
        yDelta *= WheelFactor;
        xDelta = yDelta;
    }

    if (event->modifiers() & Qt::ControlModifier && !(event->modifiers() & Qt::ShiftModifier)) {
        // Zoom vertical
        double level = yDelta / 12.0;
        semitoneHeight = std::pow(1.1, level) * semitoneHeight;
        update();
    } else if (event->modifiers() & Qt::ControlModifier && event->modifiers() & Qt::ShiftModifier) {
        // Zoom time
        double level = xDelta / 12.0;
        secondWidth = std::pow(1.1, level) * secondWidth;
        update();
    } else {
        if (event->modifiers() & Qt::ShiftModifier) {
            setTimeAxisCenterAndSyncScrollbar(centerTime - (xDelta * ScrollFactorX) / secondWidth);
        } else {
            setF0CenterAndSyncScrollbar(centerPitch + (yDelta * ScrollFactorY) / semitoneHeight);
        }
    }

    event->accept();
}

void F0Widget::mouseMoveEvent(QMouseEvent *event) {
    if (draggingStartPos.x() >= 0 && draggingMode != None) {
        if (!dragging) {
            // Start drag condition
            if ((event->pos() - draggingStartPos).manhattanLength() >= QApplication::startDragDistance()) {
                dragging = true;
                setCursor(Qt::SplitVCursor);
            }
        } else {
            if (draggingButton == Qt::LeftButton) {
                if (draggingMode == Note) {
                    // Drag note
                    draggingNoteInCents = event->modifiers() & Qt::ShiftModifier;
                    if (!snapToKey)
                        draggingNoteInCents = !draggingNoteInCents;
                }
                update();
            } else if (draggingButton == Qt::RightButton) {
            }
        }
    }
    update();

    event->accept();
}

void F0Widget::mouseDoubleClickEvent(QMouseEvent *event) {
    splitNoteUnderMouse();

    event->accept();
}

void F0Widget::mousePressEvent(QMouseEvent *event) {
    draggingStartPos = event->pos();
    draggingButton = event->button();
    MiniNoteInterval noteInterval{-1, -1};
    if (mouseOnNote(event->pos(), &noteInterval) && !noteInterval.value.isRest) {
        draggingMode = Note;
        draggingNoteStartPitch = pitchOnWidgetY(event->y());
        draggingNoteInterval = {noteInterval.low, noteInterval.high};
        draggingNoteBeginCents = std::isnan(noteInterval.value.cents) ? 0 : noteInterval.value.cents;
        draggingNoteBeginPitch = noteInterval.value.pitch;
    } else
        draggingMode = None;
}

void F0Widget::mouseReleaseEvent(QMouseEvent *event) {
    // Commit changes
    if (dragging) {
        switch (draggingMode) {
            case Note: {
                if (draggingNoteInCents) {
                    shiftDraggedNoteByPitch(pitchOnWidgetY(event->y()) - draggingNoteStartPitch);
                } else {
                    setDraggedNotePitch(pitchOnWidgetY(event->y()) + 0.5); // Key center pitch -> key bottom pitch
                }
                setCursor(Qt::ArrowCursor);
                break;
            }

            case None:
                break;
        }
    }

    draggingStartPos = {-1, -1};
    draggingButton = Qt::NoButton;
    draggingMode = None;
    dragging = false;
    update();
}


void F0Widget::contextMenuEvent(QContextMenuEvent *event) {
    MiniNoteInterval noteInterval{-1, -1};
    if (mouseOnNote(event->pos(), &contextMenuNoteInterval)) {
        // Has to determine whether some actions should be enabled
        setNoteContextMenuEntriesEnabled();
        noteMenu->exec(event->globalPos());
    } else if (QRectF(KeyWidth, TimelineHeight, width() - KeyWidth - ScrollBarWidth, height() - TimelineHeight)
                   .contains(event->pos())) {
        // In piano roll but not on note
        bgMenu->exec(event->globalPos());
    }
}

void F0Widget::setTimeAxisCenterAndSyncScrollbar(double time) {
    centerTime = std::clamp(time, std::get<0>(timeRange), std::get<1>(timeRange));
    horizontalScrollBar->setValue(centerTime * 1000);
}

void F0Widget::setF0CenterAndSyncScrollbar(double pitch) {
    centerPitch = clampPitchToF0Bounds ? std::clamp(pitch, std::get<0>(pitchRange), std::get<1>(pitchRange)) : pitch;
    verticalScrollBar->setValue(centerPitch * 100);
}

void F0Widget::onHorizontalScroll(int value) {
    centerTime = value / 1000.0;
    update();
}

void F0Widget::onVerticalScroll(int value) {
    centerPitch = value / 100.0;
    update();
}
