#include "DsTextDocBridge.h"
#include <dstools/DsKeys.h>

namespace dstools {

QList<IntervalLayer> DsTextDocBridge::extractIntervalLayers(const DsTextDocument& doc) {
    QList<IntervalLayer> result;
    for (const auto& layer : doc.layers) {
        result.append(layer);
    }
    return result;
}

void DsTextDocBridge::mergeIntervalLayers(DsTextDocument& doc, const QList<IntervalLayer>& layers) {
    for (const auto& editorLayer : layers) {
        IntervalLayer* target = nullptr;
        for (auto& layer : doc.layers) {
            if (layer.name == editorLayer.name) {
                target = &layer;
                break;
            }
        }
        if (!target) {
            doc.layers.push_back(editorLayer);
        } else {
            target->boundaries = editorLayer.boundaries;
            target->type = editorLayer.type;
        }
    }
}

std::shared_ptr<pitchlabeler::DsPitchDocument> DsTextDocBridge::toPitchDoc(const DsTextDocument& doc,
                                                                           TimePos totalDurationUs) {
    auto file = std::make_shared<pitchlabeler::DsPitchDocument>();

    for (const auto& curve : doc.curves) {
        if (curve.name == QString::fromUtf8(dstools::keys::layers::pitch)) {
            file->f0.values = curve.values;
            file->f0.timestep = curve.timestep;
            if (file->f0.timestep <= 0 && !file->f0.values.empty()) {
                file->f0.timestep = 5000;
            }
        }
    }

    for (const auto& layer : doc.layers) {
        if (layer.name == QString::fromUtf8(dstools::keys::layers::midi)) {
            for (size_t i = 0; i + 1 < layer.boundaries.size(); i += 2) {
                auto note = pitchlabeler::DsPitchDocument::deserializeNote(layer.boundaries[i].text);
                if (note) {
                    note->start = layer.boundaries[i].pos;
                    file->notes.push_back(std::move(*note));
                } else if (!layer.boundaries[i].text.isEmpty() && layer.boundaries[i + 1].text.isEmpty()) {
                    pitchlabeler::Note oldNote;
                    oldNote.start = layer.boundaries[i].pos;
                    oldNote.name = layer.boundaries[i].text;
                    oldNote.duration = layer.boundaries[i + 1].pos - layer.boundaries[i].pos;
                    file->notes.push_back(std::move(oldNote));
                }
            }
        }
        if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive)) {
            for (size_t i = 0; i < layer.boundaries.size(); ++i) {
                if (layer.boundaries[i].text.isEmpty())
                    continue;
                pitchlabeler::Phone phone;
                phone.start = layer.boundaries[i].pos;
                phone.symbol = layer.boundaries[i].text;
                phone.duration = (i + 1 < layer.boundaries.size())
                                     ? layer.boundaries[i + 1].pos - layer.boundaries[i].pos
                                     : totalDurationUs - layer.boundaries[i].pos;
                file->phones.push_back(std::move(phone));
            }
        }
    }

    return file;
}

Result<void> DsTextDocBridge::fromPitchDoc(DsTextDocument& doc, const pitchlabeler::DsPitchDocument& pdoc) {
    auto validationResult = pdoc.validate();
    if (!validationResult) {
        return Result<void>::Error("DsPitchDocument validation failed: " + validationResult.error());
    }

    for (auto& curve : doc.curves) {
        if (curve.name == QString::fromUtf8(dstools::keys::layers::pitch)) {
            curve.values = pdoc.f0.values;
            curve.timestep = pdoc.f0.timestep;
            break;
        }
    }

    for (auto& layer : doc.layers) {
        if (layer.name == QString::fromUtf8(dstools::keys::layers::midi)) {
            layer.boundaries.clear();
            for (const auto& note : pdoc.notes) {
                Boundary start;
                start.pos = note.start;
                start.text = pitchlabeler::DsPitchDocument::serializeNote(note);
                layer.boundaries.push_back(start);

                Boundary end;
                end.pos = note.end();
                layer.boundaries.push_back(end);
            }
            layer.sortBoundaries();
        }
        if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive)) {
            layer.boundaries.clear();
            for (size_t pi = 0; pi < pdoc.phones.size(); ++pi) {
                Boundary b;
                b.id = static_cast<int>(pi + 1);
                b.pos = pdoc.phones[pi].start;
                b.text = pdoc.phones[pi].symbol;
                layer.boundaries.push_back(b);
            }
            layer.sortBoundaries();
        }
    }
    return Result<void>::Ok();
}

Result<void> DsTextDocBridge::verifyLayerRoundtrip(const DsTextDocument& original, const DsTextDocument& restored) {
    if (original.layers.size() != restored.layers.size())
        return Result<void>::Error("Layer count mismatch: " + std::to_string(original.layers.size()) + " vs " +
                                   std::to_string(restored.layers.size()));

    for (size_t li = 0; li < original.layers.size(); ++li) {
        const auto& orig = original.layers[li];
        const auto& rest = restored.layers[li];

        if (orig.name != rest.name)
            return Result<void>::Error("Layer name mismatch at index " + std::to_string(li) + ": " +
                                       orig.name.toStdString() + " vs " + rest.name.toStdString());

        if (orig.boundaries.size() != rest.boundaries.size())
            return Result<void>::Error("Boundary count mismatch in layer '" + orig.name.toStdString() +
                                       "': " + std::to_string(orig.boundaries.size()) + " vs " +
                                       std::to_string(rest.boundaries.size()));

        for (size_t bi = 0; bi < orig.boundaries.size(); ++bi) {
            if (orig.boundaries[bi].pos != rest.boundaries[bi].pos)
                return Result<void>::Error("Boundary pos mismatch in layer '" + orig.name.toStdString() +
                                           "' at index " + std::to_string(bi));
        }
    }

    return Result<void>::Ok();
}

Result<void> DsTextDocBridge::verifyPitchDocRoundtrip(const DsTextDocument& original,
                                                      const pitchlabeler::DsPitchDocument& restored) {
    for (const auto& curve : original.curves) {
        if (curve.name == QString::fromUtf8(dstools::keys::layers::pitch)) {
            if (curve.values.size() != restored.f0.values.size())
                return Result<void>::Error("Pitch curve value count mismatch: " + std::to_string(curve.values.size()) +
                                           " vs " + std::to_string(restored.f0.values.size()));
            break;
        }
    }

    for (const auto& layer : original.layers) {
        if (layer.name == QString::fromUtf8(dstools::keys::layers::midi)) {
            if (layer.boundaries.size() != restored.notes.size() * 2)
                return Result<void>::Error("MIDI boundary count mismatch: " + std::to_string(layer.boundaries.size()) +
                                           " vs " + std::to_string(restored.notes.size() * 2));
            break;
        }
    }

    return Result<void>::Ok();
}

} // namespace dstools