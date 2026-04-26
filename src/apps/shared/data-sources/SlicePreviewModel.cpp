#include "SlicePreviewModel.h"

#include <dstools/IEditorDataSource.h>
#include <dstools/DsTextTypes.h>
#include <dstools/DsKeys.h>

#include <dsfw/PipelineContext.h>

#include <QJsonDocument>
#include <QJsonObject>

namespace dstools {

void SlicePreviewModel::setDataSource(IEditorDataSource *source) {
    m_source = source;
    invalidate();
}

void SlicePreviewModel::invalidate() {
    m_valid = false;
    m_rows.clear();
}

void SlicePreviewModel::refresh() {
    if (!m_source)
        return;

    if (m_valid)
        return;

    buildRows();
    m_valid = true;
}

static void extractPhonemeData(const IntervalLayer &phonemeLayer,
                               const IntervalLayer *graphemeLayer,
                               SlicePreviewRow &row) {
    QStringList phs, durs;
    for (size_t i = 0; i < phonemeLayer.boundaries.size(); ++i) {
        phs << phonemeLayer.boundaries[i].text;
        if (i + 1 < phonemeLayer.boundaries.size()) {
            double durSec = usToSec(phonemeLayer.boundaries[i + 1].pos
                                    - phonemeLayer.boundaries[i].pos);
            durs << QString::number(durSec, 'f', 6);
        }
    }
    row.phSeq = phs.join(' ');
    row.phDur = durs.join(' ');

    if (graphemeLayer && !graphemeLayer->boundaries.empty()) {
        QStringList spans;
        int wordIdx = 1;
        for (size_t i = 0; i < phonemeLayer.boundaries.size(); ++i) {
            const auto &ph = phonemeLayer.boundaries[i];
            if (ph.text.isEmpty())
                continue;
            while (wordIdx < static_cast<int>(graphemeLayer->boundaries.size()) &&
                   ph.pos >= graphemeLayer->boundaries[wordIdx].pos) {
                ++wordIdx;
            }
            spans << QString::number(wordIdx);
        }
        row.wordSpan = spans.join(' ');
    }
}

static void extractPhNumData(const IntervalLayer &phNumLayer,
                             SlicePreviewRow &row) {
    QStringList nums;
    for (const auto &b : phNumLayer.boundaries) {
        if (!b.text.isEmpty())
            nums << b.text;
    }
    row.phNum = nums.join(' ');
}

static void extractMidiData(const IntervalLayer &midiLayer,
                            SlicePreviewRow &row) {
    QStringList notes;
    QStringList nd;
    for (const auto &b : midiLayer.boundaries) {
        QJsonParseError err;
        auto jdoc = QJsonDocument::fromJson(b.text.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && jdoc.isObject()) {
            auto obj = jdoc.object();
            notes << obj["n"].toString();
            double durSec = usToSec(static_cast<TimePos>(obj["d"].toDouble()));
            nd << QString::number(durSec, 'f', 6);
        } else {
            notes << b.text;
            nd << QStringLiteral("0");
        }
    }
    row.noteSeq = notes.join(' ');
    row.noteDur = nd.join(' ');
}

static void findLayers(const DsTextDocument &doc,
                       const IntervalLayer *&phonemeLayer,
                       const IntervalLayer *&graphemeLayer,
                       const IntervalLayer *&midiLayer,
                       const IntervalLayer *&phNumLayer) {
    for (const auto &layer : doc.layers) {
        if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive))
            phonemeLayer = &layer;
        else if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme))
            graphemeLayer = &layer;
        else if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::midi), Qt::CaseInsensitive)
                 || layer.name.contains(QString::fromUtf8(dstools::keys::layers::note), Qt::CaseInsensitive))
            midiLayer = &layer;
        else if (layer.name == QString::fromUtf8(dstools::keys::layers::phNum))
            phNumLayer = &layer;
    }
}

void SlicePreviewModel::buildRows() {
    m_rows.clear();

    if (!m_source)
        return;

    const auto sliceIds = m_source->sliceIds();

    for (const auto &sliceId : sliceIds) {
        auto *ctxPtr = m_source->context(sliceId);
        if (ctxPtr && ctxPtr->status != PipelineContext::Status::Active)
            continue;

        SlicePreviewRow row;
        row.name = sliceId;

        auto docResult = m_source->loadSlice(sliceId);
        if (docResult) {
            const auto &doc = docResult.value();

            const IntervalLayer *phonemeLayer = nullptr;
            const IntervalLayer *graphemeLayer = nullptr;
            const IntervalLayer *midiLayer = nullptr;
            const IntervalLayer *phNumLayer = nullptr;
            findLayers(doc, phonemeLayer, graphemeLayer, midiLayer, phNumLayer);

            if (phonemeLayer)
                extractPhonemeData(*phonemeLayer, graphemeLayer, row);
            if (phNumLayer)
                extractPhNumData(*phNumLayer, row);
            if (midiLayer)
                extractMidiData(*midiLayer, row);
        }

        const auto dirty = m_source->dirtyLayers(sliceId);
        if (!dirty.empty()) {
            row.dirty = dirty.join(QStringLiteral(", "));
        }

        row.hasMissing = row.phSeq.isEmpty() || row.phDur.isEmpty();
        m_rows.push_back(std::move(row));
    }
}

} // namespace dstools