#include <dstools/LayerSerialization.h>

#include <dstools/DsKeys.h>
#include <dstools/TimePos.h>

#include <nlohmann/json.hpp>

namespace dstools {

    namespace {

        TextBoundaryLayer parseBoundariesArray(const nlohmann::json &j) {
            TextBoundaryLayer result;
            for (const auto &item : j) {
                Boundary b;
                b.id = item.value("id", 0);
                b.pos = item.value("pos", int64_t(0));
                b.text = QString::fromStdString(item.value("text", ""));
                result.push_back(b);
            }
            return result;
        }

        nlohmann::json serializeBoundariesArray(const TextBoundaryLayer &boundaries) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto &b : boundaries) {
                arr.push_back({
                    {"id", b.id},
                    {"pos", b.pos},
                    {"text", b.text.toStdString()}
                });
            }
            return arr;
        }

        PitchCurveLayer parsePitchArray(const nlohmann::json &j) {
            PitchCurveLayer curve;
            if (j.is_array()) {
                curve.values.reserve(j.size());
                for (const auto &v : j)
                    curve.values.push_back(hzToMhz(v.get<double>()));
            } else if (j.is_object() && j.contains("f0_seq")) {
                double timestepSec = j.value("f0_timestep", 0.005);
                curve.timestep = secToUs(timestepSec);

                if (j["f0_seq"].is_string()) {
                    const auto f0Str = QString::fromStdString(j["f0_seq"].get<std::string>());
                    const auto vals = f0Str.split(' ', Qt::SkipEmptyParts);
                    curve.values.reserve(vals.size());
                    for (const auto &v : vals)
                        curve.values.push_back(hzToMhz(v.toDouble()));
                }
            }
            return curve;
        }

        nlohmann::json serializePitchArray(const PitchCurveLayer &curve) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto &v : curve.values)
                arr.push_back(mhzToHz(v));
            return arr;
        }

        PhonemeNumberLayer parsePhNumValues(const nlohmann::json &j) {
            PhonemeNumberLayer result;
            if (j.is_object() && j.contains("values")) {
                for (const auto &v : j["values"])
                    result.push_back(v.get<int>());
            }
            return result;
        }

        nlohmann::json serializePhNumValues(const PhonemeNumberLayer &nums) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto &n : nums)
                arr.push_back(n);
            return {{"values", arr}};
        }

        SliceBoundaryLayer parseSlicesArray(const nlohmann::json &j) {
            SliceBoundaryLayer result;
            for (const auto &item : j) {
                SliceBoundary s;
                s.id = item.value("id", 0);
                s.pos = item.value("in", int64_t(0));
                s.endPos = item.value("out", int64_t(0));
                result.push_back(s);
            }
            return result;
        }

        nlohmann::json serializeSlicesArray(const SliceBoundaryLayer &slices) {
            nlohmann::json arr = nlohmann::json::array();
            int idx = 1;
            for (const auto &s : slices) {
                arr.push_back({
                    {"id", QString("%1").arg(idx++, 3, 10, QChar('0')).toStdString()},
                    {"in", s.pos},
                    {"out", s.endPos},
                    {"status", "active"}
                });
            }
            return arr;
        }

        MidiNoteLayer parseMidiArray(const nlohmann::json &j) {
            MidiNoteLayer result;
            for (const auto &item : j) {
                MidiNote n;
                n.pitch = item.value("pitch", 0);
                n.onset = item.value("onset", 0.0);
                n.duration = item.value("duration", 0.0);
                n.voiced = item.value("voiced", false);
                result.push_back(n);
            }
            return result;
        }

        nlohmann::json serializeMidiArray(const MidiNoteLayer &notes) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto &n : notes) {
                arr.push_back({
                    {"pitch", n.pitch},
                    {"onset", n.onset},
                    {"duration", n.duration},
                    {"voiced", n.voiced}
                });
            }
            return arr;
        }

        StringLayer parseTextValue(const nlohmann::json &j) {
            if (j.is_object() && j.contains("text"))
                return j["text"].get<std::string>();
            if (j.is_string())
                return j.get<std::string>();
            return {};
        }

        nlohmann::json serializeTextValue(const StringLayer &text) {
            return {{"text", text}};
        }

    } // namespace

    Result<LayerDataVariant> parseLayerData(const LayerData &data, const QString &layerType) {
        try {
            const auto j = data.toJson();

            if (layerType == QString::fromUtf8(keys::layers::phoneme) ||
                layerType == QString::fromUtf8(keys::layers::grapheme)) {
                return LayerDataVariant(parseBoundariesArray(j));
            }

            if (layerType == QString::fromUtf8(keys::layers::pitch)) {
                return LayerDataVariant(parsePitchArray(j));
            }

            if (layerType == QString::fromUtf8(keys::layers::phNum)) {
                return LayerDataVariant(parsePhNumValues(j));
            }

            if (layerType == QStringLiteral("slices")) {
                return LayerDataVariant(parseSlicesArray(j));
            }

            if (layerType == QString::fromUtf8(keys::layers::midi) ||
                layerType == QString::fromUtf8(keys::layers::note)) {
                return LayerDataVariant(parseMidiArray(j));
            }

            if (layerType == QString::fromUtf8(keys::layers::text) ||
                layerType == QStringLiteral("sentence")) {
                return LayerDataVariant(parseTextValue(j));
            }

            return Result<LayerDataVariant>::Error(
                "Unknown layer type: " + layerType.toStdString());
        } catch (const std::exception &e) {
            return Result<LayerDataVariant>::Error(
                std::string("parseLayerData failed: ") + e.what());
        }
    }

    LayerData serializeLayerData(const LayerDataVariant &variant) {
        return std::visit([](const auto &value) -> LayerData {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, TextBoundaryLayer> ||
                          std::is_same_v<T, PhonemeBoundaryLayer>) {
                return LayerData::fromJson(serializeBoundariesArray(value));
            } else if constexpr (std::is_same_v<T, PitchCurveLayer>) {
                return LayerData::fromJson(serializePitchArray(value));
            } else if constexpr (std::is_same_v<T, PhonemeNumberLayer>) {
                return LayerData::fromJson(serializePhNumValues(value));
            } else if constexpr (std::is_same_v<T, SliceBoundaryLayer>) {
                return LayerData::fromJson(serializeSlicesArray(value));
            } else if constexpr (std::is_same_v<T, MidiNoteLayer>) {
                return LayerData::fromJson(serializeMidiArray(value));
            } else if constexpr (std::is_same_v<T, StringLayer>) {
                return LayerData::fromJson(serializeTextValue(value));
            } else if constexpr (std::is_same_v<T, TranscriptionLayer>) {
                nlohmann::json arr = nlohmann::json::array();
                for (const auto &entry : value) {
                    arr.push_back({
                        {"name", entry.name.toStdString()},
                        {"phSeq", entry.phSeq.toStdString()},
                        {"phDur", entry.phDur.toStdString()},
                        {"phNum", entry.phNum.toStdString()},
                        {"noteSeq", entry.noteSeq.toStdString()},
                        {"noteDur", entry.noteDur.toStdString()},
                        {"wordSpan", entry.wordSpan.toStdString()},
                        {"startSec", entry.startSec},
                        {"endSec", entry.endSec},
                        {"dirty", entry.dirty}
                    });
                }
                return LayerData::fromJson(arr);
            } else {
                return LayerData();
            }
        }, variant);
    }

} // namespace dstools