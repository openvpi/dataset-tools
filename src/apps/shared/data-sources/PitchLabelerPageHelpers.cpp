#include "PitchLabelerPageHelpers.h"

#include "BatchProcessDialog.h"
#include "EnginePool.h"
#include "ModelConfigHelper.h"

#include <dstools/DsKeys.h>
#include "PitchLabelerPage.h"

#include <dstools/DsPitchDocument.h>
#include <PitchEditor.h>
#include <QCheckBox>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QtConcurrent/QtConcurrent>
#include <dsfw/IModelProvider.h>
#include <dsfw/Log.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/Constants.h>
#include <dstools/CurveTools.h>
#include <dstools/DsTextTypes.h>
#include <dstools/PitchUtils.h>
#include <sndfile.hh>

namespace dstools {

    void pitchLabelerApplyPitchResult(IEditorDataSource *source, std::shared_ptr<pitchlabeler::DsPitchDocument> &currentFile,
                                      pitchlabeler::PitchEditor *editor, const QString &sliceId,
                                      const std::vector<float> &f0, float timestep) {
        if (!source)
            return;

        if (f0.empty())
            return;

        auto makeMidiNote = [](double pitch, const Game::GameNote &gn) -> QString {
            Q_UNUSED(pitch)
            auto parsed = parseNoteName(QString::fromStdString(std::to_string(pitch)));
            return parsed.valid ? midiToNoteString(parsed.midiNumber) : QStringLiteral("rest");
        };

        if (!currentFile)
            currentFile = std::make_shared<pitchlabeler::DsPitchDocument>();
        currentFile->f0.values = f0;
        currentFile->f0.timestep = secToUs(timestep);
        editor->loadDsPitchDocument(currentFile);

        auto result = source->loadSlice(sliceId);
        if (!result) {
            DSFW_LOG_ERROR(
                "pitch",
                ("Failed to load slice for pitch result: " + sliceId.toStdString() + " - " + result.error()).c_str());
            return;
        }
        DsTextDocument doc = std::move(result.value());

        CurveLayer *pitchCurve = nullptr;
        for (auto &curve : doc.curves) {
            if (curve.name == QString::fromUtf8(dstools::keys::layers::pitch)) {
                pitchCurve = &curve;
                break;
            }
        }
        if (!pitchCurve) {
            doc.curves.push_back({});
            pitchCurve = &doc.curves.back();
            pitchCurve->name = QString::fromUtf8(dstools::keys::layers::pitch);
        }
        pitchCurve->values = f0;
        pitchCurve->timestep = secToUs(static_cast<double>(timestep));

        auto saveResult = source->saveSlice(sliceId, doc);
        if (!saveResult) {
            dsfw::widgets::ToastNotification::show(editor, dsfw::widgets::ToastType::Error,
                                                   QObject::tr("音高数据保存失败"));
        }
    }

    void pitchLabelerApplyMidiResult(IEditorDataSource *source, std::shared_ptr<pitchlabeler::DsPitchDocument> &currentFile,
                                     pitchlabeler::PitchEditor *editor, const QString &sliceId,
                                     const std::vector<Game::GameNote> &notes) {
        if (!source)
            return;

        auto gameNoteMidiName = [](double pitch) -> QString { return midiToNoteString(pitch); };

        auto result = source->loadSlice(sliceId);
        if (!result) {
            DSFW_LOG_ERROR(
                "pitch",
                ("Failed to load slice for MIDI result: " + sliceId.toStdString() + " - " + result.error()).c_str());
            return;
        }
        DsTextDocument doc = std::move(result.value());

        if (!notes.empty()) {
            IntervalLayer *midiLayer = nullptr;
            for (auto &layer : doc.layers) {
                if (layer.name == QString::fromUtf8(dstools::keys::layers::midi)) {
                    midiLayer = &layer;
                    break;
                }
            }
            if (!midiLayer) {
                doc.layers.push_back({});
                midiLayer = &doc.layers.back();
                midiLayer->name = QString::fromUtf8(dstools::keys::layers::midi);
                midiLayer->type = QString::fromUtf8(dstools::keys::layers::note);
            }

            midiLayer->boundaries.clear();
            int id = 1;
            for (const auto &gn : notes) {
                pitchlabeler::Note n;
                n.start = static_cast<int64_t>(gn.onset * 1000000.0);
                n.duration = static_cast<int64_t>(gn.duration * 1000000.0);
                n.name = gn.voiced ? gameNoteMidiName(gn.pitch) : QStringLiteral("rest");

                Boundary b;
                b.id = id++;
                b.pos = n.start;
                b.text = pitchlabeler::DsPitchDocument::serializeNote(n);
                midiLayer->boundaries.push_back(std::move(b));
            }
        } else {
            doc.layers.erase(std::remove_if(doc.layers.begin(), doc.layers.end(),
                                            [](const IntervalLayer &l) { return l.name == QString::fromUtf8(dstools::keys::layers::midi); }),
                             doc.layers.end());
        }

        if (auto res = source->saveSlice(sliceId, doc); !res.ok()) {
            DSFW_LOG_ERROR(
                "pitch",
                ("Failed to save slice after MIDI extraction: " + sliceId.toStdString() + " - " + res.error()).c_str());
            dsfw::widgets::ToastNotification::show(
                editor, dsfw::widgets::ToastType::Error,
                QObject::tr("MIDI 保存失败: %1").arg(QString::fromStdString(res.error())), 5000);
            return;
        }

        source->clearDirtyLayers(sliceId, {QString::fromUtf8(dstools::keys::layers::midi)});

        if (!currentFile)
            currentFile = std::make_shared<pitchlabeler::DsPitchDocument>();
        currentFile->notes.clear();
        for (const auto &note : notes) {
            pitchlabeler::Note n;
            n.start = static_cast<int64_t>(note.onset * 1000000.0);
            n.duration = static_cast<int64_t>(note.duration * 1000000.0);
            n.name = note.voiced ? gameNoteMidiName(note.pitch) : QStringLiteral("rest");
            currentFile->notes.push_back(std::move(n));
        }
        editor->loadDsPitchDocument(currentFile);
    }

    void pitchLabelerRunBatchExtract(PitchLabelerPage *page) {
        if (!page)
            return;

        auto *source = page->source();
        if (!source) {
            QMessageBox::warning(page, QObject::tr("批量提取"), QObject::tr("请先打开工程。"));
            return;
        }

        page->rmvpeRef() = page->enginePool()->acquire<Rmvpe::Rmvpe>(QLatin1String(dstools::keys::engines::pitchExtraction));
        page->gameRef() = page->enginePool()->acquire<Game::Game>(QLatin1String(dstools::keys::engines::midiTranscription));

        bool hasRmvpe = page->enginePool()->isOpen<Rmvpe::Rmvpe>(QLatin1String(dstools::keys::engines::pitchExtraction));
        bool hasGame = page->enginePool()->isOpen<Game::Game>(QLatin1String(dstools::keys::engines::midiTranscription));

        if (!hasRmvpe && !hasGame) {
            QMessageBox::warning(page, QObject::tr("批量提取"),
                                 QObject::tr("RMVPE 和 GAME 模型均未加载。请在设置中配置模型路径。"));
            return;
        }

        if (page->isBatchRunning()) {
            QMessageBox::information(page, QObject::tr("批量提取"), QObject::tr("推理正在运行中，请稍候。"));
            return;
        }

        const auto ids = source->sliceIds();
        if (ids.isEmpty()) {
            QMessageBox::information(page, QObject::tr("批量提取"), QObject::tr("没有可处理的切片。"));
            return;
        }

        auto *dlg = new BatchProcessDialog(QObject::tr("批量提取音高 + MIDI"), page);
        dlg->setAttribute(Qt::WA_DeleteOnClose);

        auto *extractPitch = new QCheckBox(QObject::tr("提取音高 (RMVPE)"), dlg);
        extractPitch->setChecked(hasRmvpe);
        extractPitch->setEnabled(hasRmvpe);
        dlg->addParamWidget(extractPitch);

        auto *extractMidi = new QCheckBox(QObject::tr("提取 MIDI (GAME)"), dlg);
        extractMidi->setChecked(hasGame);
        extractMidi->setEnabled(hasGame);
        dlg->addParamWidget(extractMidi);

        auto *skipExisting = new QCheckBox(QObject::tr("跳过已有结果的切片"), dlg);
        skipExisting->setChecked(true);
        dlg->addParamWidget(skipExisting);

        auto *allowNonAlignCb = new QCheckBox(QObject::tr("允许非 align 模式（音素/词典缺失时）"), dlg);
        allowNonAlignCb->setChecked(false);
        auto *allowNonAlignWarn = new QLabel(QObject::tr("⚠ 不推荐：非 align 模式下 MIDI 转录质量会显著下降。\n"
                                                         "建议先在设置中配置 ph_num 特殊词和词典后再使用 align 模式。"),
                                             dlg);
        allowNonAlignWarn->setWordWrap(true);
        allowNonAlignWarn->setStyleSheet(QStringLiteral("color: #e67e22; font-size: 12px; margin-left: 12px;"));
        dlg->addParamWidget(allowNonAlignCb);
        dlg->addParamWidget(allowNonAlignWarn);

        dlg->appendLog(QObject::tr("总切片数: %1").arg(ids.size()));

        auto *rmvpe = page->rmvpeRef();
        auto *game = page->gameRef();
        auto rmvpeAlive = page->aliveToken(QLatin1String(dstools::keys::engines::pitchExtraction)).token();
        auto gameAlive = page->aliveToken(QLatin1String(dstools::keys::engines::midiTranscription)).token();
        auto *src = source;
        QPointer<PitchLabelerPage> guard(page);
        page->loadPhNumCalculator();
        auto phNumCalc = page->phNumCalc();

        QObject::connect(
            dlg, &BatchProcessDialog::started, page,
            [dlg, rmvpe, game, rmvpeAlive, gameAlive, src, ids, guard, extractPitch, extractMidi, skipExisting,
             allowNonAlignCb, phNumCalc]() {
                if (!guard) {
                    dlg->finish(0, 0, 0);
                    return;
                }
                guard->setBatchRunning(true);
                guard->extractPitchAction()->setEnabled(false);
                guard->extractMidiAction()->setEnabled(false);
                bool doPitch = extractPitch->isChecked();
                bool doMidi = extractMidi->isChecked();
                bool skip = skipExisting->isChecked();

                (void) QtConcurrent::run([dlg, rmvpe, game, rmvpeAlive, gameAlive, src, ids, guard, doPitch, doMidi,
                                          skip, phNumCalc, allowNonAlignCb]() {
                    int processed = 0;
                    int skipped = 0;
                    int errors = 0;
                    int idx = 0;
                    try {
                        for (const auto &sliceId : ids) {
                            if (dlg->isCancelled())
                                break;
                            if ((doPitch && (!rmvpeAlive || !*rmvpeAlive)) || (doMidi && (!gameAlive || !*gameAlive)))
                                break;

                            QMetaObject::invokeMethod(
                                dlg, [dlg, idx, total = ids.size()]() { dlg->setProgress(idx, total); },
                                Qt::QueuedConnection);

                            QString audioPath = src->validatedAudioPath(sliceId);
                            if (audioPath.isEmpty()) {
                                ++skipped;
                                QMetaObject::invokeMethod(
                                    dlg,
                                    [dlg, sliceId]() {
                                        dlg->appendLog(QObject::tr("[SKIP] %1 (missing audio)").arg(sliceId));
                                    },
                                    Qt::QueuedConnection);
                                ++idx;
                                continue;
                            }

                            if (skip) {
                                auto result = src->loadSlice(sliceId);
                                if (result) {
                                    bool hasData = false;
                                    for (const auto &curve : result.value().curves) {
                                        if (curve.name == QString::fromUtf8(dstools::keys::layers::pitch) && !curve.values.empty()) {
                                            hasData = true;
                                            break;
                                        }
                                    }
                                    if (!hasData) {
                                        for (const auto &layer : result.value().layers) {
                                            if (layer.name == QString::fromUtf8(dstools::keys::layers::midi) && !layer.boundaries.empty()) {
                                                hasData = true;
                                                break;
                                            }
                                        }
                                    }
                                    if (hasData) {
                                        ++skipped;
                                        QMetaObject::invokeMethod(
                                            dlg,
                                            [dlg, sliceId]() {
                                                dlg->appendLog(QObject::tr("[SKIP] %1 (existing data)").arg(sliceId));
                                            },
                                            Qt::QueuedConnection);
                                        ++idx;
                                        continue;
                                    }
                                }
                            }

                            int errorsBefore = errors;

                            if (doPitch && rmvpe) {
                                std::vector<Rmvpe::RmvpeRes> results;
                                auto result = rmvpe->get_f0(audioPath.toStdWString(), 0.01f, results, nullptr);
                                if (result && !results.empty()) {
                                    const auto &res = results[0];
                                    std::vector<float> f0Mhz(res.f0.size());
                                    for (size_t i = 0; i < res.f0.size(); ++i) {
                                        f0Mhz[i] = res.f0[i] * 1000.0f;
                                    }
                                    float timestep = 0.01f;
#ifdef _WIN32
                                    auto pathStrB = audioPath.toStdWString();
#else
                                    auto pathStrB = audioPath.toStdString();
#endif
                                    SndfileHandle sf(pathStrB.c_str());
                                    if (sf && sf.samplerate() > 0) {
                                        const int sampleRate = sf.samplerate();
                                        const int64_t audioFrames = sf.frames();
                                        const TimePos dstTimestepUs = hopSizeToTimestep(constants::kDefaultHopSize, sampleRate);
                                        const int alignLength = expectedFrameCount(audioFrames, constants::kDefaultHopSize);
                                        f0Mhz = resampleCurve(f0Mhz, secToUs(0.01), dstTimestepUs, alignLength);
                                        timestep = static_cast<float>(usToSec(dstTimestepUs));
                                    }
                                    QMetaObject::invokeMethod(
                                        guard.data(),
                                        [guard, sliceId, f0Mhz = std::move(f0Mhz), timestep]() {
                                            if (guard) {
                                                guard->applyPitchResult(sliceId, f0Mhz, timestep);
                                            }
                                        },
                                        Qt::QueuedConnection);
                                } else {
                                    ++errors;
                                    DSFW_LOG_ERROR("infer", (std::string("Batch pitch extraction failed: ") +
                                                             sliceId.toStdString() + " - " +
                                                             (result ? "empty results" : result.error()))
                                                                .c_str());
                                    QMetaObject::invokeMethod(
                                        dlg,
                                        [dlg, sliceId]() {
                                            dlg->appendLog(
                                                QObject::tr("[ERROR] %1: pitch extraction failed").arg(sliceId));
                                        },
                                        Qt::QueuedConnection);
                                }
                            }

                            if (doMidi && game) {
                                Game::AlignInput alignInput;
                                bool useAlign = false;

                                auto loadResult = src->loadSlice(sliceId);
                                if (loadResult) {
                                    const auto &doc = loadResult.value();
                                    for (const auto &layer : doc.layers) {
                                        if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive) &&
                                            !layer.boundaries.empty()) {
                                            const auto &bnd = layer.boundaries;
                                            for (size_t i = 0; i < bnd.size(); ++i) {
                                                if (!bnd[i].text.isEmpty()) {
                                                    alignInput.phSeq.push_back(bnd[i].text.toStdString());
                                                    TimePos dur = (i + 1 < bnd.size())
                                                                      ? bnd[i + 1].pos - bnd[i].pos
                                                                      : secToUs(EditorPageBase::audioDurationSec(doc)) -
                                                                            bnd[i].pos;
                                                    alignInput.phDur.push_back(static_cast<float>(usToSec(dur)));
                                                }
                                            }
                                            if (!alignInput.phSeq.empty()) {
                                                bool hasPhNum = false;
                                                for (const auto &l : doc.layers) {
                                                    if (l.name == QString::fromUtf8(dstools::keys::layers::phNum) && !l.boundaries.empty()) {
                                                        for (const auto &b : l.boundaries) {
                                                            bool ok = false;
                                                            int val = b.text.toInt(&ok);
                                                            alignInput.phNum.push_back(ok ? val : 0);
                                                        }
                                                        long long sum = 0;
                                                        for (int v : alignInput.phNum)
                                                            sum += v;
                                                        if (static_cast<size_t>(sum) == alignInput.phSeq.size()) {
                                                            hasPhNum = true;
                                                        } else {
                                                            alignInput.phNum.clear();
                                                        }
                                                        break;
                                                    }
                                                }
                                                if (!hasPhNum && phNumCalc.isLoaded()) {
                                                    QStringList phSeqStr;
                                                    for (const auto &ph : alignInput.phSeq)
                                                        phSeqStr << QString::fromStdString(ph);
                                                    auto calcResult =
                                                        phNumCalc.calculate(phSeqStr.join(QChar(' ')));
                                                    if (calcResult.ok()) {
                                                        const auto parts =
                                                            calcResult->split(QChar(' '), Qt::SkipEmptyParts);
                                                        for (const auto &part : parts) {
                                                            bool ok = false;
                                                            int val = part.toInt(&ok);
                                                            alignInput.phNum.push_back(ok ? val : 0);
                                                        }
                                                        long long sum = 0;
                                                        for (int v : alignInput.phNum)
                                                            sum += v;
                                                        if (static_cast<size_t>(sum) == alignInput.phSeq.size()) {
                                                            hasPhNum = true;
                                                        } else {
                                                            alignInput.phNum.clear();
                                                        }
                                                    }
                                                }
                                                useAlign = hasPhNum;
                                                if (!hasPhNum && !alignInput.phSeq.empty()) {
                                                    if (allowNonAlignCb && allowNonAlignCb->isChecked()) {
                                                        useAlign = false;
                                                        QMetaObject::invokeMethod(
                                                            dlg,
                                                            [dlg, sliceId]() {
                                                                dlg->appendLog(
                                                                    QObject::tr(
                                                                        "[WARN] %1: ph_num 不可用，使用非 align 模式")
                                                                        .arg(sliceId));
                                                            },
                                                            Qt::QueuedConnection);
                                                    } else {
                                                        ++skipped;
                                                        QMetaObject::invokeMethod(
                                                            dlg,
                                                            [dlg, sliceId]() {
                                                                dlg->appendLog(
                                                                    QObject::tr("[SKIP] %1: ph_num "
                                                                                "不可用，且未允许非 align 模式")
                                                                        .arg(sliceId));
                                                            },
                                                            Qt::QueuedConnection);
                                                        ++idx;
                                                        continue;
                                                    }
                                                }
                                            }
                                            break;
                                        }
                                    }
                                }

                                std::vector<Game::GameNote> notes;
                                dstools::Result<void> midiResult = Err("Not executed");

                                if (useAlign) {
                                    alignInput.wavPath = std::filesystem::path(audioPath.toStdWString());
                                    Game::AlignOptions options;
                                    std::vector<Game::AlignedNote> alignedNotes;
                                    midiResult = game->align(alignInput, options, alignedNotes);
                                    if (midiResult) {
                                        float currentTime = 0.0f;
                                        for (const auto &an : alignedNotes) {
                                            Game::GameNote gn;
                                            bool isRest = an.name.empty() || an.name == "rest";
                                            gn.voiced = !isRest;
                                            if (!isRest) {
                                                auto parsed = dstools::parseNoteName(QString::fromStdString(an.name));
                                                gn.pitch = parsed.valid ? parsed.midiNumber : 60.0f;
                                            } else {
                                                gn.pitch = 0.0f;
                                            }
                                            gn.onset = currentTime;
                                            gn.duration = static_cast<float>(an.duration);
                                            notes.push_back(gn);
                                            currentTime += static_cast<float>(an.duration);
                                        }
                                    }
                                } else {
                                    midiResult = game->getNotes(audioPath.toStdWString(), notes, nullptr);
                                }

                                if (midiResult) {
                                    QMetaObject::invokeMethod(
                                        guard.data(),
                                        [guard, sliceId, notes = std::move(notes)]() {
                                            if (guard)
                                                guard->applyMidiResult(sliceId, notes);
                                        },
                                        Qt::QueuedConnection);
                                } else {
                                    ++errors;
                                    DSFW_LOG_ERROR("infer", (std::string("Batch MIDI transcription failed: ") +
                                                             sliceId.toStdString() + " - " + midiResult.error())
                                                                .c_str());
                                    QMetaObject::invokeMethod(
                                        dlg,
                                        [dlg, sliceId]() {
                                            dlg->appendLog(
                                                QObject::tr("[ERROR] %1: MIDI transcription failed").arg(sliceId));
                                        },
                                        Qt::QueuedConnection);
                                }
                            }

                            if (errors == errorsBefore) {
                                QMetaObject::invokeMethod(
                                    dlg, [dlg, sliceId]() { dlg->appendLog(QObject::tr("[OK] %1").arg(sliceId)); },
                                    Qt::QueuedConnection);
                                ++processed;
                            }
                            ++idx;
                        }
                    } catch (const std::exception &e) {
                        ++errors;
                        DSFW_LOG_ERROR("infer", ("Batch pitch/MIDI exception: " + std::string(e.what())).c_str());
                        QMetaObject::invokeMethod(
                            dlg,
                            [dlg, eMsg = std::string(e.what())]() {
                                dlg->appendLog(QObject::tr("[ERROR] Exception: %1").arg(QString::fromStdString(eMsg)));
                            },
                            Qt::QueuedConnection);
                    }
                    QMetaObject::invokeMethod(
                        dlg, [dlg, processed, skipped, errors]() { dlg->finish(processed, skipped, errors); },
                        Qt::QueuedConnection);
                    QMetaObject::invokeMethod(
                        guard.data(),
                        [guard]() {
                            if (guard) {
                                guard->setBatchRunning(false);
                                guard->extractPitchAction()->setEnabled(true);
                                guard->extractMidiAction()->setEnabled(true);
                            }
                        },
                        Qt::QueuedConnection);
                });
            });

        QObject::connect(dlg, &BatchProcessDialog::cancelled, page, [guard]() {
            if (guard) {
                guard->setBatchRunning(false);
                guard->extractPitchAction()->setEnabled(true);
                guard->extractMidiAction()->setEnabled(true);
            }
        });

        dlg->show();
    }

} // namespace dstools
