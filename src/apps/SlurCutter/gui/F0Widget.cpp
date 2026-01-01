#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <cmath>


#include "DsSentence.h"
#include "F0Widget.h"

#include <QSettings>

namespace SlurCutter {
    F0Widget::F0Widget(QWidget *parent)
        : QFrame(parent), draggingNoteInterval(0, 0), contextMenuNoteInterval(0, 0, {}) {
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

        auto boldMenuItemFont = noteMenu->font();
        boldMenuItemFont.setBold(true);

        noteMenu->addSeparator();
        noteMenuGlidePrompt = new QAction("Ornament: Glide");
        noteMenuGlidePrompt->setFont(boldMenuItemFont);
        noteMenuGlidePrompt->setEnabled(false);
        noteMenu->addAction(noteMenuGlidePrompt);
        noteMenuSetGlideType = new QActionGroup(this);
        noteMenuSetGlideNone = new QAction("None");
        noteMenuSetGlideUp = new QAction("Up");
        noteMenuSetGlideDown = new QAction("Down");
        noteMenu->addAction(noteMenuSetGlideType->addAction(noteMenuSetGlideNone));
        noteMenu->addAction(noteMenuSetGlideType->addAction(noteMenuSetGlideUp));
        noteMenu->addAction(noteMenuSetGlideType->addAction(noteMenuSetGlideDown));
        noteMenuSetGlideNone->setCheckable(true);
        noteMenuSetGlideUp->setCheckable(true);
        noteMenuSetGlideDown->setCheckable(true);
        noteMenuSetGlideNone->setChecked(true);

        bgMenu = new QMenu(this);
        bgMenuReloadSentence = new QAction("&Discard changes and reload current sentence");
        bgMenu_CautionPrompt = new QAction("Use the following with caution");
        bgMenuConvRestsToNormal = new QAction("Convert all Rests to normal notes");
        bgMenu_OptionPrompt = new QAction("Options");
        bgMenuSnapByDefault = new QAction("&Snap to piano keys by default");
        bgMenuShowPitchTextOverlay = new QAction("Show pitch text &overlay");
        bgMenuShowPhonemeTexts = new QAction("Show &phonemes");
        bgMenuShowCrosshairAndPitch = new QAction("Show &crosshair and pitch");
        bgMenu_ModePrompt = new QAction("Edit Mode");
        bgMenuModeNote = new QAction("&Note");
        bgMenuModeGlide = new QAction("Ornament: &Glide");
        bgMenu_OptionPrompt->setFont(boldMenuItemFont);
        bgMenu_OptionPrompt->setDisabled(true);
        bgMenu_CautionPrompt->setFont(boldMenuItemFont);
        bgMenu_CautionPrompt->setDisabled(true);
        bgMenu_ModePrompt->setFont(boldMenuItemFont);
        bgMenu_ModePrompt->setDisabled(true);
        bgMenuShowPitchTextOverlay->setCheckable(true);
        bgMenuShowPhonemeTexts->setCheckable(true);
        bgMenuShowCrosshairAndPitch->setCheckable(true);
        bgMenuSnapByDefault->setCheckable(true);
        bgMenuModeNote->setCheckable(true);
        bgMenuModeGlide->setCheckable(true);
        bgMenu->addAction(bgMenuReloadSentence);
        bgMenu->addSeparator();
        bgMenu->addAction(bgMenu_ModePrompt);
        bgMenu->addAction(bgMenuModeNote);
        bgMenu->addAction(bgMenuModeGlide);
        bgMenu->addSeparator();
        bgMenu->addAction(bgMenu_CautionPrompt);
        bgMenu->addAction(bgMenuConvRestsToNormal);
        bgMenu->addSeparator();
        bgMenu->addAction(bgMenu_OptionPrompt);
        bgMenu->addAction(bgMenuSnapByDefault);
        bgMenu->addAction(bgMenuShowPitchTextOverlay);
        bgMenu->addAction(bgMenuShowPhonemeTexts);
        bgMenu->addAction(bgMenuShowCrosshairAndPitch);

        // Mode selector
        selectedDragMode = Note;
        bgMenuModeGroup = new QActionGroup(this);
        bgMenuModeNote->setData(Note);
        bgMenuModeGlide->setData(Glide);
        bgMenuModeGroup->addAction(bgMenuModeNote);
        bgMenuModeGroup->addAction(bgMenuModeGlide);
        bgMenuModeGroup->setExclusive(true);
        bgMenuModeNote->setChecked(true);

        // Connect signals
        connect(horizontalScrollBar, &QScrollBar::valueChanged, this, &F0Widget::onHorizontalScroll);
        connect(verticalScrollBar, &QScrollBar::valueChanged, this, &F0Widget::onVerticalScroll);

        connect(noteMenu, &QMenu::aboutToShow, this, &F0Widget::setMenuFromCurrentNote);
        connect(noteMenuMergeLeft, &QAction::triggered, this, &F0Widget::mergeCurrentSlurToLeftNode);
        connect(noteMenuToggleRest, &QAction::triggered, this, &F0Widget::toggleCurrentNoteRest);
        connect(noteMenuSetGlideType, &QActionGroup::triggered, this, &F0Widget::setCurrentNoteGlideType);

        connect(bgMenuReloadSentence, &QAction::triggered, this, &F0Widget::requestReloadSentence);
        connect(bgMenuConvRestsToNormal, &QAction::triggered, this, &F0Widget::convertAllRestsToNormal);
        connect(bgMenuShowPitchTextOverlay, &QAction::triggered, [=] {
            showPitchTextOverlay = bgMenuShowPitchTextOverlay->isChecked();
            update();
        });
        connect(bgMenuShowPhonemeTexts, &QAction::triggered, [=] {
            showPhonemeTexts = bgMenuShowPhonemeTexts->isChecked();
            update();
        });
        connect(bgMenuSnapByDefault, &QAction::triggered, [=] {
            snapToKey = bgMenuSnapByDefault->isChecked();
            update();
        });
        connect(bgMenuShowCrosshairAndPitch, &QAction::triggered, [=] {
            showCrosshairAndPitch = bgMenuShowCrosshairAndPitch->isChecked();
            update();
        });
        connect(bgMenuModeNote, &QAction::triggered, this, &F0Widget::modeChanged);
        connect(bgMenuModeGlide, &QAction::triggered, this, &F0Widget::modeChanged);
    }

    F0Widget::~F0Widget() {
    }

    void F0Widget::setDsSentenceContent(const QJsonObject &content) {
        hasError = false;
        clear();

        DsSentence sentence = loadDsSentencesFromJsonObj(content);

        auto f0Seq = sentence.f0_seq.split(" ", Qt::SkipEmptyParts);
        foreach (auto f0, f0Seq) {
            f0Values.append(FrequencyToMidiNote(f0.toDouble()));
        }
        f0Timestep = sentence.f0_timestep.toDouble();

        auto phSeq = sentence.ph_seq.split(" ", Qt::SkipEmptyParts);
        auto phDur = sentence.ph_dur.split(" ", Qt::SkipEmptyParts);
        auto phNum = sentence.ph_num.split(" ", Qt::SkipEmptyParts);
        auto noteSeq = sentence.note_seq.split(" ", Qt::SkipEmptyParts);
        auto noteDur = sentence.note_dur.split(" ", Qt::SkipEmptyParts);
        auto text = sentence.text.split(" ", Qt::SkipEmptyParts);
        auto slur = sentence.note_slur.split(" ", Qt::SkipEmptyParts);
        auto glide = sentence.note_glide.split(" ");

        QVector<bool> isRest(noteSeq.size(), false);

        // Sanity check
        if (noteDur.size() != noteSeq.size() || phDur.size() != phSeq.size()) {
            setErrorStatusText(QString("Invalid DS file! Inconsistent element count\n"
                                       "note_dur: %1\n"
                                       "note_seq: %2\n"
                                       "ph_dur: %3\n"
                                       "ph_seq: %4\n"
                                       "slur(optional): %5")
                                   .arg(noteDur.size())
                                   .arg(noteSeq.size())
                                   .arg(phDur.size())
                                   .arg(phSeq.size())
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
        auto phBegin = 0.0;
        int ph_j = 0;
        for (int i = 0; i < noteSeq.size(); i++) {
            MiniNote note;
            note.duration = noteDur[i].toDouble();
            // Parse note pitch
            auto notePitch = noteSeq[i];
            if (notePitch.contains(QRegularExpression(R"((\+|\-))"))) {
                auto splitPitch = notePitch.split(QRegularExpression(R"((\+|\-))"));
                note.pitch = NoteNameToMidiNote(splitPitch[0]);
                note.cents = splitPitch[1].toDouble();
                note.cents *= notePitch[splitPitch[0].length()] == '+' ? 1 : -1;
            } else {
                note.pitch = NoteNameToMidiNote(noteSeq[i]);
                note.cents = NAN;
            }
            note.isSlur = !slur.empty() && slur[i].toInt() > 0;
            note.isRest = isRest[i];
            if (glide.size() - 1 < i || glide[i] == "none")
                note.glide = GlideStyle::None;
            else if (glide[i] == "up")
                note.glide = GlideStyle::Up;
            else if (glide[i] == "down")
                note.glide = GlideStyle::Down;
            if (showPhonemeTexts) {
                while (ph_j < phDur.size() && phBegin >= noteBegin - 0.01 &&
                       phBegin < noteBegin + note.duration - 0.01) {
                    MiniPhoneme ph;
                    ph.begin = phBegin;
                    ph.duration = phDur[ph_j].toDouble();
                    ph.ph = phSeq[ph_j];
                    note.phonemes.append(ph);
                    ph_j++;
                    phBegin += ph.duration;
                }
            }
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

    void F0Widget::loadConfig(const QSettings *cfg) {
        snapToKey = cfg->value("F0Widget/snapToKey", snapToKey).toBool();
        showPitchTextOverlay = cfg->value("F0Widget/showPitchTextOverlay", showPitchTextOverlay).toBool();
        showPhonemeTexts = cfg->value("F0Widget/showPhonemeTexts", showPhonemeTexts).toBool();
        showCrosshairAndPitch = cfg->value("F0Widget/showCrosshairAndPitch", showCrosshairAndPitch).toBool();

        bgMenuShowPitchTextOverlay->setChecked(showPitchTextOverlay);
        bgMenuShowPhonemeTexts->setChecked(showPhonemeTexts);
        bgMenuSnapByDefault->setChecked(snapToKey);
        bgMenuShowCrosshairAndPitch->setChecked(showCrosshairAndPitch);

        update();
    }

    void F0Widget::pullConfig(QSettings &cfg) const {
        cfg.setValue("F0Widget/snapToKey", snapToKey);
        cfg.setValue("F0Widget/showPitchTextOverlay", showPitchTextOverlay);
        cfg.setValue("F0Widget/showPhonemeTexts", showPhonemeTexts);
        cfg.setValue("F0Widget/showCrosshairAndPitch", showCrosshairAndPitch);
    }

    void F0Widget::clear() {
        hasError = false;
        errorStatusText = "";
        midiIntervals.clear();
        f0Values.clear();
        isEmpty = true;
        update();
    }

    F0Widget::ReturnedDsString F0Widget::getSavedDsStrings() const {
        ReturnedDsString ret;
        for (const auto &i : midiIntervals.intervals()) {
            ret.note_dur += QString::number(i.value.duration, 'g', 3) + ' ';
            ret.note_slur += i.value.isSlur ? "1 " : "0 ";
            ret.note_seq += i.value.isRest ? "rest "
                            : std::isnan(i.value.cents)
                                ? MidiNoteToNoteName(i.value.pitch) + ' '
                                : PitchToNotePlusCentsString(i.value.pitch + 0.01 * i.value.cents) + ' ';
            if (i.value.isRest) {
                // rest notes must have no glides
                ret.note_glide += "none ";
            } else {
                switch (i.value.glide) {
                    case GlideStyle::None:
                        ret.note_glide += "none ";
                        break;
                    case GlideStyle::Up:
                        ret.note_glide += "up ";
                        break;
                    case GlideStyle::Down:
                        ret.note_glide += "down ";
                        break;
                }
            }
        }
        ret.note_dur = ret.note_dur.trimmed();
        ret.note_seq = ret.note_seq.trimmed();
        ret.note_slur = ret.note_slur.trimmed();
        ret.note_glide = ret.note_glide.trimmed();
        return ret;
    }

    bool F0Widget::empty() const {
        return isEmpty;
    }

    void F0Widget::setPlayHeadPos(const double pos) {
        qDebug() << pos;
        playheadPos = pos;
        const auto w = width() - KeyWidth - ScrollBarWidth;
        const double leftTime = centerTime - w / 2 / secondWidth;
        const double rightTime = centerTime + w / 2 / secondWidth;
        if (pos > rightTime || pos < leftTime)
            setTimeAxisCenterAndSyncScrollbar(pos + (rightTime - leftTime) / 2);
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
        const int octave = noteName.right(1).toInt();
        return 11 + 12 * octave + noteNameToMidiNote[noteName.left(noteName.size() - 1)];
    }

    QString F0Widget::MidiNoteToNoteName(int midiNote) {
        // A0 = MIDI 21, C8 = 108. Return empty when out of range
        static const char *NoteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        if (midiNote > 108 || midiNote < 21)
            return QString();
        midiNote -= 12;
        return QString("%1%2").arg(NoteNames[midiNote % 12]).arg(midiNote / 12);
    }

    double F0Widget::FrequencyToMidiNote(const double f) {
        return 69 + 12 * log2(f / 440);
    }

    std::tuple<int, double> F0Widget::PitchToNoteAndCents(const double pitch) {
        if (pitch == 0)
            return {0, 0};
        int note = std::round(pitch);
        double cents = (pitch - note) * 100;
        return {note, cents};
    }

    QString F0Widget::PitchToNotePlusCentsString(const double pitch) {
        if (pitch == 0)
            return "rest";
        const int note = std::round(pitch);
        const int cents = std::round((pitch - note) * 100);
        if (cents == 0)
            return MidiNoteToNoteName(note);
        return QString("%1%2%3").arg(MidiNoteToNoteName(note)).arg(cents >= 0 ? "+" : "").arg(cents);
    }

    auto F0Widget::refF0IndexRange(const double startTime, const double endTime) const -> std::tuple<size_t, size_t> {
        return {std::min(static_cast<size_t>(floor(std::max(0.0, startTime / f0Timestep))),
                         static_cast<size_t>(f0Values.size()) - 1),
                std::min(static_cast<size_t>(ceil(endTime / f0Timestep)), static_cast<size_t>(f0Values.size()) - 1)};
    }


    bool F0Widget::mouseOnNote(const QPoint &mousePos, MiniNoteInterval *returnNote) const {
        auto mouseTime = timeOnWidgetX(mousePos.x());
        const auto mousePitch = pitchOnWidgetY(mousePos.y());

        const auto matchedNotes = midiIntervals.findOverlappingIntervals({mouseTime, mouseTime}, true);
        if (matchedNotes.empty())
            return false;

        for (auto &i : matchedNotes) {
            const double pitch = i.value.pitch + (std::isnan(i.value.cents) ? 0 : i.value.cents / 100);
            if (mousePitch >= pitch - 0.5 && mousePitch <= pitch + 0.5) {
                if (returnNote)
                    *returnNote = i;
                return true;
            }
        }
        return false;
    }

    double F0Widget::pitchOnWidgetY(const int y) const {
        const auto h = height() - TimelineHeight;
        return centerPitch + h / 2 / semitoneHeight - (y - TimelineHeight) / semitoneHeight;
    }

    double F0Widget::timeOnWidgetX(const int x) const {
        const auto w = width() - KeyWidth - ScrollBarWidth;
        return centerTime - w / 2 / secondWidth + (x - KeyWidth) / secondWidth;
    }

    void F0Widget::setNoteContextMenuEntriesEnabled() const {
        auto noteInterval = contextMenuNoteInterval;
        auto rightIntervals =
            midiIntervals.findOverlappingIntervals({noteInterval.high, noteInterval.high + 0.1}, false);

        // Only slurs can be merged to left
        noteMenuMergeLeft->setEnabled(noteInterval.value.isSlur);
    }

    void F0Widget::splitNoteUnderMouse() {
        MiniNoteInterval noteInterval{-1, -1};
        const auto cursorPos = mapFromGlobal(QCursor::pos());

        if (mouseOnNote(cursorPos, &noteInterval) && !noteInterval.value.isRest) {
            auto time = timeOnWidgetX(cursorPos.x());

            MiniNote leftNote{noteInterval.value}, rightNote{noteInterval.value};
            leftNote.duration = time - noteInterval.low;
            rightNote.duration = noteInterval.high - time;
            rightNote.isSlur = true;
            leftNote.phonemes.clear();
            rightNote.phonemes.clear();
            for (auto &ph : noteInterval.value.phonemes) {
                if (ph.begin >= noteInterval.low && ph.begin < time) {
                    leftNote.phonemes.append(ph);
                } else if (ph.begin >= time && ph.begin < noteInterval.high) {
                    rightNote.phonemes.append(ph);
                }
            }

            midiIntervals.remove(noteInterval);
            midiIntervals.insert({noteInterval.low, time, leftNote});
            midiIntervals.insert({time, noteInterval.high, rightNote});

            update();
        }
    }

    void F0Widget::shiftDraggedNoteByPitch(const double pitchDelta) {
        auto intervals =
            midiIntervals.findInnerIntervals({std::get<0>(draggingNoteInterval), std::get<1>(draggingNoteInterval)});
        if (intervals.empty())
            return;

        auto &note = intervals[0].value;
        const double addedCents = draggingNoteInCents ? (std::isnan(note.cents) ? 0 : note.cents) : 0;
        double semitoneDelta;
        const double newCents = 100 * modf(pitchDelta + 0.01 * addedCents, &semitoneDelta);

        note.cents = newCents;
        note.pitch += semitoneDelta;

        midiIntervals.remove(intervals[0]);
        midiIntervals.insert(intervals[0]);
    }

    void F0Widget::setDraggedNotePitch(const int pitch) {
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

    void F0Widget::setDraggedNoteGlide(const GlideStyle style) {
        auto intervals =
            midiIntervals.findInnerIntervals({std::get<0>(draggingNoteInterval), std::get<1>(draggingNoteInterval)});
        if (intervals.empty())
            return;

        auto &note = intervals[0].value;
        note.glide = style;

        midiIntervals.remove(intervals[0]);
        midiIntervals.insert(intervals[0]);
    }

    void F0Widget::modeChanged() {
        // Fuck this what the hell is this bloody cast chain
        selectedDragMode = static_cast<decltype(Note)>(static_cast<QAction *>(sender())->data().toInt());
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

    void F0Widget::setMenuFromCurrentNote() const {
        auto [duration, pitch, cents, text, phonemes, isSlur, isRest, glide] = contextMenuNoteInterval.value;
        noteMenuSetGlideType->setEnabled(!isRest);
        if (isRest) {
            noteMenuSetGlideNone->setChecked(true);
        } else {
            if (glide == GlideStyle::None) {
                noteMenuSetGlideNone->setChecked(true);
            } else if (glide == GlideStyle::Up) {
                noteMenuSetGlideUp->setChecked(true);
            } else if (glide == GlideStyle::Down) {
                noteMenuSetGlideDown->setChecked(true);
            }
        }
    }

    void F0Widget::mergeCurrentSlurToLeftNode(const bool checked) {
        Q_UNUSED(checked);

        auto noteInterval = contextMenuNoteInterval;

        const auto intervals =
            midiIntervals.findOverlappingIntervals({noteInterval.low - 0.1, noteInterval.low}, false);
        if (intervals.empty())
            return;
        auto leftNode = intervals.back();

        midiIntervals.remove(leftNode);
        midiIntervals.remove(noteInterval);

        leftNode.value.duration += noteInterval.value.duration;
        leftNode.high = noteInterval.high;
        leftNode.value.phonemes.append(noteInterval.value.phonemes);

        midiIntervals.insert(leftNode);
        update();
    }

    void F0Widget::toggleCurrentNoteRest() {
        auto noteInterval = contextMenuNoteInterval;

        midiIntervals.remove(noteInterval);
        noteInterval.value.isRest = !noteInterval.value.isRest;
        midiIntervals.insert(noteInterval);
        update();
    }

    void F0Widget::setCurrentNoteGlideType(const QAction *action) {
        GlideStyle style;
        if (action == this->noteMenuSetGlideUp) {
            style = GlideStyle::Up;
        } else if (action == this->noteMenuSetGlideDown) {
            style = GlideStyle::Down;
        } else {
            style = GlideStyle::None;
        }
        auto noteInterval = contextMenuNoteInterval;
        midiIntervals.remove(noteInterval);
        noteInterval.value.glide = style;
        midiIntervals.insert(noteInterval);
        update();
    }

    void F0Widget::paintEvent(QPaintEvent *event) {
        QPainter painter(this);
        QFontMetrics fm = fontMetrics();
        int lh = fm.lineSpacing();

        do {
            // Fill dark grey background
            painter.fillRect(rect(), QColor(40, 40, 40));

            // Convenience references
            auto w = width(), h = height();
            constexpr Qt::GlobalColor keyColor[] = {Qt::white, Qt::black, Qt::white, Qt::black, Qt::white, Qt::white,
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
            double keyReferenceY = h / 2 - (1 - fmod(centerPitch, 1)) * semitoneHeight; // Top of center pitch's key
            int lowestPitch = floor(centerPitch) - ceil((h - keyReferenceY) / semitoneHeight);
            double lowestPitchY = keyReferenceY + (static_cast<int>(centerPitch - lowestPitch) + 0.5) * semitoneHeight;

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
            QVector<QPair<QPointF, QString>> noteDescription;
            QVector<QPair<QPointF, QString>> phonemeTexts;
            static constexpr QColor NoteColors[] = {QColor(106, 164, 234), QColor(60, 113, 219)};
            auto leftBoundaryIntervals = midiIntervals.findIntervalsContainPoint(leftTime, false);
            auto deltaLeftTime = 0.0;
            if (!leftBoundaryIntervals.empty() && !leftBoundaryIntervals.front().value.phonemes.empty())
                deltaLeftTime = leftBoundaryIntervals.front().value.phonemes.back().duration * secondWidth;
            for (auto &i : midiIntervals.findOverlappingIntervals({leftTime - deltaLeftTime, rightTime}, false)) {

                if (i.value.pitch == 0)
                    continue; // Skip rests (pitch 0)
                auto rec =
                    QRectF((i.low - leftTime) * secondWidth,
                           lowestPitchY -
                               (i.value.pitch + (std::isnan(i.value.cents) ? 0 : i.value.cents) * 0.01 - lowestPitch) *
                                   semitoneHeight,
                           i.value.duration * secondWidth, semitoneHeight);
                if (rec.bottom() < 0 || rec.top() > h)
                    continue;
                if (!i.value.isRest) {
                    QString noteDescText;
                    painter.setPen(Qt::black);
                    painter.fillRect(rec, NoteColors[i.value.isSlur]);
                    painter.drawRect(rec);
                    painter.drawLine(rec.left(), rec.center().y(), rec.right(), rec.center().y());
                    noteDescText += PitchToNotePlusCentsString(i.value.pitch +
                                                               0.01 * (std::isnan(i.value.cents) ? 0 : i.value.cents));
                    if (i.value.glide != GlideStyle::None) {
                        /*
                         * Definitions of glide types which cause the difference
                         * between prepending and appending:
                         * 1. Up
                         * The pitch glides up from the beginning, TOWARDS the main note.
                         * 2. Down
                         * The pitch glides down at the end, FROM the main note.
                         */
                        if (i.value.glide == GlideStyle::Up)
                            noteDescText.prepend("↗");
                        if (i.value.glide == GlideStyle::Down)
                            noteDescText.append("↘");
                    }
                    // Defer the drawing of deviation text to prevent the right side notes overlapping with them
                    if (!noteDescText.isEmpty()) {
                        noteDescription.append({rec.topLeft() + QPointF(0, -3), noteDescText});
                    }
                } else {
                    auto pen = painter.pen();
                    pen.setStyle(Qt::DashLine);
                    pen.setColor(NoteColors[0]);
                    pen.setWidth(1);
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
                if (showPhonemeTexts) {
                    for (auto &[ph, begin, duration] : i.value.phonemes) {
                        auto phRec = QRectF((begin - leftTime) * secondWidth, rec.y() + semitoneHeight,
                                            duration * secondWidth, lh + 3);
                        auto pen = painter.pen();
                        pen.setStyle(Qt::SolidLine);
                        pen.setColor(QColor(200, 200, 200, 255));
                        pen.setWidth(2);
                        painter.setPen(pen);
                        painter.drawLine(phRec.left() + 1, phRec.top() + 1, phRec.left() + 1, phRec.bottom());
                        pen.setStyle(Qt::NoPen);
                        painter.setPen(pen);
                        auto brush = painter.brush();
                        brush.setColor(QColor(200, 200, 200, 80));
                        brush.setStyle(Qt::SolidPattern);
                        painter.setBrush(brush);
                        painter.drawRect(phRec);
                        painter.setBrush(Qt::NoBrush);
                        phonemeTexts.append({phRec.topLeft() + QPointF(NotePadding, lh - 3), ph});
                    }
                }
            }

            // Drag box / hover box (Do not coexist)
            MiniNoteInterval note{-1, -1};
            if (dragging) {
                switch (draggingMode) {
                    case Note: {
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
                            auto dragPitch = mousePitch - draggingNoteStartPitch + draggingNoteBeginCents * 0.01 +
                                             draggingNoteBeginPitch;
                            auto rec = QRectF(noteLeft, lowestPitchY - (dragPitch - lowestPitch) * semitoneHeight,
                                              noteRight - noteLeft, semitoneHeight);
                            painter.fillRect(rec, QColor(255, 255, 255, 60));
                            painter.drawLine(rec.left(), rec.center().y(), rec.right(), rec.center().y());
                            painter.drawRect(rec);
                            painter.drawText(rec.topLeft() + QPointF(0, -3), PitchToNotePlusCentsString(dragPitch));
                        } else {
                            auto dragPitch = floor(mousePitch + 0.5); // Key center pitch -> key bottom pitch
                            auto rec = QRectF(noteLeft, lowestPitchY - (dragPitch - lowestPitch) * semitoneHeight,
                                              noteRight - noteLeft, semitoneHeight);
                            painter.fillRect(rec, QColor(255, 255, 255, 60));
                            painter.drawRect(rec);
                        }
                        break;
                    }

                    case Glide: {
                        auto pos = mapFromGlobal(QCursor::pos());
                        auto mousePitch = pitchOnWidgetY(pos.y());
                        auto noteLeft = (std::get<0>(draggingNoteInterval) - leftTime) * secondWidth,
                             noteRight = (std::get<1>(draggingNoteInterval) - leftTime) * secondWidth;
                        auto noteCenter = (noteLeft + noteRight) / 2;
                        auto pen = painter.pen();
                        pen.setColor(Qt::white);
                        pen.setStyle(Qt::SolidLine);
                        pen.setWidth(0);
                        painter.setPen(pen);
                        painter.drawLine(
                            noteCenter, lowestPitchY - (draggingNoteBeginPitch - lowestPitch - 0.5) * semitoneHeight,
                            noteCenter, lowestPitchY - (std::round(mousePitch) - lowestPitch - 0.5) * semitoneHeight);
                        break;
                    }

                    default:
                        break;
                }
            } else if (mouseOnNote(mapFromGlobal(QCursor::pos()), &note)) {
                auto rec =
                    QRectF((note.low - leftTime) * secondWidth,
                           lowestPitchY - (note.value.pitch +
                                           (std::isnan(note.value.cents) ? 0 : note.value.cents) * 0.01 - lowestPitch) *
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
                double f0X = (std::get<0>(visibleF0) - leftTime / f0Timestep) * f0Timestep * secondWidth;
                path.moveTo(f0X, lowestPitchY - (f0Values[std::get<0>(visibleF0)] - lowestPitch) * semitoneHeight);
                for (int i = std::get<0>(visibleF0) + 1; i < std::get<1>(visibleF0); i++) {
                    f0X += f0Timestep * secondWidth;
                    path.lineTo(f0X, lowestPitchY - (f0Values[i] - lowestPitch) * semitoneHeight);
                }
                painter.setPen(Qt::red);
                painter.drawPath(path);

                lowestPitchY -= semitoneHeight / 2;
            }

            // Note description (Note+-Cents, glide, ...) text
            painter.setPen(Qt::white);
            foreach (auto &i, noteDescription) {
                painter.drawText(i.first, i.second);
            }
            if (showPhonemeTexts) {
                foreach (auto &i, phonemeTexts) {
                    painter.drawText(i.first, i.second);
                }
            }

            painter.translate(-KeyWidth, 0);
            painter.setClipRect(0, 0, width(), h);
            w += KeyWidth;

            // Debug text
            painter.translate(0, -TimelineHeight);
            h += TimelineHeight;
            painter.setClipRect(0, 0, w, h);
            auto mousePos = mapFromGlobal(QCursor::pos());
            auto mousePitch = pitchOnWidgetY(mousePos.y());
            if (showPitchTextOverlay) {
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

            // PlayHead (playHeadPos)
            painter.setPen(QColor(255, 180, 0));
            painter.drawLine((playheadPos - leftTime) * secondWidth + KeyWidth, TimelineHeight,
                             (playheadPos - leftTime) * secondWidth + KeyWidth, h);

            // Mouse hover crosshair
            if (showCrosshairAndPitch) {
                QString crosshairText = PitchToNotePlusCentsString(mousePitch);
                QRectF crosshairTextRect = painter.fontMetrics().boundingRect(crosshairText);
                painter.setClipRect(KeyWidth, TimelineHeight, w - KeyWidth, h - TimelineHeight);
                painter.setPen(QColor(255, 255, 255, 96));
                painter.drawLine(mousePos.x(), TimelineHeight, mousePos.x(), h);
                painter.drawLine(0, mousePos.y(), w, mousePos.y());
                painter.translate(mousePos);
                painter.fillRect(QRectF(CrosshairTextMargin, -CrosshairTextMargin,
                                        CrosshairTextPadding * 2 + crosshairTextRect.width(),
                                        -(CrosshairTextPadding * 2 + crosshairTextRect.height())),
                                 QColor(0, 0, 0, 160));
                painter.setPen(QColor(255, 255, 255));
                painter.drawText(crosshairTextRect.translated(QPointF(CrosshairTextPadding + CrosshairTextMargin,
                                                                      -(CrosshairTextPadding + CrosshairTextMargin))),
                                 PitchToNotePlusCentsString(mousePitch));
                painter.translate(-mousePos);
            }

            // Piano keys
            painter.translate(0, TimelineHeight);
            h -= TimelineHeight;
            painter.setClipRect(0, 0, w, h);
            auto prevFont = painter.font();
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
                    auto font = prevFont;
                    font.setBold(true);
                    painter.setFont(font);
                } else {
                    painter.setFont(prevFont);
                }
                painter.drawText(rec, Qt::AlignRight | Qt::AlignVCenter, MidiNoteToNoteName(lowestPitch));
            } while (lowestPitchY > 0 && lowestPitch <= 108);
            painter.setFont(prevFont);

            painter.translate(0, -TimelineHeight);
            painter.setClipRect(0, 0, w, height());
            h += TimelineHeight;
        } while (false);

        painter.end();

        QFrame::paintEvent(event);
    }

    void F0Widget::resizeEvent(QResizeEvent *event) {
        const auto w = width();
        // Update the scrollbars
        const auto h = height();
        horizontalScrollBar->setGeometry(0, 0, w, HorizontalScrollHeight);
        horizontalScrollBar->setPageStep(w / secondWidth * 1000);
        verticalScrollBar->setGeometry(w - ScrollBarWidth, TimelineHeight, ScrollBarWidth, h - TimelineHeight);
        verticalScrollBar->setPageStep((h - TimelineHeight) / semitoneHeight * 100);

        QFrame::resizeEvent(event);
    }

    void F0Widget::wheelEvent(QWheelEvent *event) {
        auto xDelta = event->angleDelta().x(), yDelta = event->angleDelta().y();

        if (xDelta == 0 && yDelta % 120 == 0) {
            constexpr double WheelFactor = 0.1;
            yDelta *= WheelFactor;
            xDelta = yDelta;
        }

        if (event->modifiers() & Qt::ControlModifier && !(event->modifiers() & Qt::ShiftModifier)) {
            // Zoom vertical
            const double level = yDelta / 12.0;
            semitoneHeight = std::pow(1.1, level) * semitoneHeight;
            update();
        } else if (event->modifiers() & Qt::ControlModifier && event->modifiers() & Qt::ShiftModifier) {
            // Zoom time
            const double level = xDelta / 12.0;
            secondWidth = std::pow(1.1, level) * secondWidth;
            update();
        } else {
            if (event->modifiers() & Qt::ShiftModifier) {
                constexpr double ScrollFactorX = 10;
                setTimeAxisCenterAndSyncScrollbar(centerTime - xDelta * ScrollFactorX / secondWidth);
            } else {
                constexpr double ScrollFactorY = 2;
                setF0CenterAndSyncScrollbar(centerPitch + yDelta * ScrollFactorY / semitoneHeight);
            }
        }

        event->accept();
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
            // You are dragging a note
            switch (selectedDragMode) {
                case Note:
                    draggingMode = Note;
                    break;
                case Glide:
                    draggingMode = Glide;
                    break;
                default:
                    break;
            }

            // This may be less useful for glide labeling but anyway
            draggingNoteStartPitch = pitchOnWidgetY(event->position().y());
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
                        shiftDraggedNoteByPitch(pitchOnWidgetY(event->position().y()) - draggingNoteStartPitch);
                    } else {
                        setDraggedNotePitch(pitchOnWidgetY(event->position().y()) +
                                            0.5); // Key center pitch -> key bottom pitch
                    }
                    setCursor(Qt::ArrowCursor);
                    break;
                }

                case Glide: {
                    const int deltaPitch = std::round(pitchOnWidgetY(event->position().y())) - draggingNoteStartPitch;
                    if (deltaPitch == 0)
                        setDraggedNoteGlide(GlideStyle::None);
                    else if (deltaPitch > 0)
                        setDraggedNoteGlide(GlideStyle::Up);
                    else if (deltaPitch < 0)
                        setDraggedNoteGlide(GlideStyle::Down);
                    setCursor(Qt::ArrowCursor);
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

    void F0Widget::setTimeAxisCenterAndSyncScrollbar(const double time) {
        centerTime = std::clamp(time, std::get<0>(timeRange), std::get<1>(timeRange));
        horizontalScrollBar->setValue(centerTime * 1000);
    }

    auto F0Widget::setF0CenterAndSyncScrollbar(const double pitch) -> void {
        centerPitch =
            clampPitchToF0Bounds ? std::clamp(pitch, std::get<0>(pitchRange), std::get<1>(pitchRange)) : pitch;
        verticalScrollBar->setValue(centerPitch * 100);
    }

    void F0Widget::onHorizontalScroll(const int value) {
        centerTime = value / 1000.0;
        update();
    }

    void F0Widget::onVerticalScroll(const int value) {
        centerPitch = value / 100.0;
        update();
    }
}