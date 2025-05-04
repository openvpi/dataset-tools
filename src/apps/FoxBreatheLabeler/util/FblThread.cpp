#include "FblThread.h"

#include <QMSystem.h>
#include <QMessageBox>

#include "textgrid.hpp"

#include <sstream>

namespace FBL {
    FblThread::FblThread(FBL *fbl, QString filename, QString wavPath, QString rawTgPath, QString outTgPath,
                         const float ap_threshold, const float ap_dur, const float sp_dur)
        : m_asr(fbl), m_filename(std::move(filename)), m_wavPath(std::move(wavPath)), m_rawTgPath(std::move(rawTgPath)),
          m_outTgPath(std::move(outTgPath)), ap_threshold(ap_threshold), ap_dur(ap_dur), sp_dur(sp_dur) {
    }

    struct Phone {
        double start, end;
        std::string text;
    };
    struct Word {
        double start{}, end{};
        std::string text;
        QList<Phone> phones;
    };

    static std::vector<std::pair<float, float>>
        find_overlapping_segments(double start, double end, const std::vector<std::pair<float, float>> &segments,
                                  float sp_dur = 0.1) {
        std::vector<std::pair<float, float>> overlapping_segments;
        std::vector<std::pair<float, float>> merged_segments;

        // Find overlapping segments
        for (const auto &segment : segments) {
            float segment_start = segment.first;
            float segment_end = segment.second;
            // Check if there is any overlap
            if (!(segment_end <= start || segment_start >= end)) {
                overlapping_segments.push_back(segment);
            }
        }

        if (overlapping_segments.empty()) {
            return overlapping_segments;
        }

        // Sort the overlapping segments based on their start time
        std::sort(overlapping_segments.begin(), overlapping_segments.end());

        // Merge adjacent segments if the gap is less than sp_dur
        float current_start = overlapping_segments[0].first;
        float current_end = overlapping_segments[0].second;

        for (size_t i = 1; i < overlapping_segments.size(); ++i) {
            float next_start = overlapping_segments[i].first;
            float next_end = overlapping_segments[i].second;

            if (next_start - current_end < sp_dur) {
                // Merge segments
                current_end = std::max(current_end, next_end);
            } else {
                // No merge, save the current segment
                merged_segments.emplace_back(current_start, current_end);
                current_start = next_start;
                current_end = next_end;
            }
        }

        // Append the last segment
        merged_segments.emplace_back(current_start, current_end);

        return merged_segments;
    }

    void FblThread::run() {
        std::string fblMsg;
        std::vector<std::pair<float, float>> segment;
        const auto fblRes = m_asr->recognize(m_wavPath.toStdString(), segment, fblMsg, ap_threshold, ap_dur);

        if (!fblRes) {
            Q_EMIT this->oneFailed(m_filename, QString::fromStdString(fblMsg));
            return;
        }

        QFile file(m_rawTgPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fblMsg = (QString("Cannot open file: filename = ") + m_rawTgPath + ", error = " + file.errorString())
                         .toStdString();
            Q_EMIT this->oneFailed(m_filename, QString::fromStdString(fblMsg));
            return;
        }

        QTextStream in(&file);
        std::string fileContent = in.readAll().toStdString();

        textgrid::TextGrid textgrid;
        std::istringstream ifs(fileContent);
        ifs >> textgrid;

        const auto wordIntervals = textgrid.GetTierAs<textgrid::IntervalTier>("words")->GetAllIntervals();
        const auto phonesIntervals = textgrid.GetTierAs<textgrid::IntervalTier>("phones")->GetAllIntervals();



        QMap<double, Word> wordsDict;
        double wordCursor = 0;
        for (const auto &interval : wordIntervals) {
            if (interval.min_time > wordCursor)
                wordsDict[wordCursor] = Word{wordCursor, interval.min_time, "", {}};
            wordsDict[interval.min_time] = Word{interval.min_time, interval.max_time, interval.text, {}};
            wordCursor = interval.max_time;
        }

        for (const auto &interval : wordIntervals) {
            const double wordStart = interval.min_time;
            const double wordEnd = interval.max_time;

            for (const auto &phone : phonesIntervals) {
                if (phone.min_time >= wordStart && phone.max_time <= wordEnd)
                    wordsDict[interval.min_time].phones.append(Phone{phone.min_time, phone.max_time, phone.text});
            }
        }

        QList<Word> out;
        for (const auto &v : wordsDict) {
            if (v.text == "SP" || v.text.empty()) {
                const auto sp = v;
                double cursor;
                const auto overlappingSegments = find_overlapping_segments(sp.start, sp.end, segment, sp_dur);
                if (overlappingSegments.empty())
                    out.append(v);
                else if (overlappingSegments.size() == 1) {
                    const auto ap = overlappingSegments[0];
                    cursor = sp.start;
                    if (sp.start + sp_dur <= ap.first && ap.first < sp.end) {
                        out.append(Word{cursor, ap.first, "SP"});
                        cursor = ap.first;
                    }

                    if (ap.second <= sp.end - sp_dur) {
                        out.append(Word{cursor, ap.second, "AP"});
                        out.append(Word{ap.second, sp.end, "SP"});
                    } else {
                        out.append(Word{cursor, sp.end, "AP"});
                    }
                } else {
                    cursor = sp.start;
                    for (int i = 0; i < overlappingSegments.size(); i++) {
                        const auto ap = overlappingSegments[i];
                        if (ap.first > cursor) {
                            out.append(Word{cursor, ap.first, "SP"});
                            cursor = ap.first;
                        }

                        if (i == 0) {
                            if (cursor < ap.first && sp.start + sp_dur <= ap.first && ap.first < sp.end) {
                                out.append(Word{cursor, ap.first, "SP"});
                                cursor = ap.first;
                            }
                            out.append(Word{cursor, ap.second, "AP"});
                            cursor = ap.second;
                        } else if (overlappingSegments.size() - 1) {
                            if (ap.second <= sp.end - sp_dur) {
                                out.append(Word{cursor, ap.second, "AP"});
                                out.append(Word{ap.second, sp.end, "SP"});
                            } else {
                                out.append(Word{cursor, sp.end, "AP"});
                            }
                        } else {
                            out.append(Word{cursor, ap.second, "AP"});
                            cursor = ap.second;
                        }
                    }
                }
            } else {
                out.append(v);
            }
        }

        textgrid::TextGrid outTg(0.0, wordCursor);

        auto tierWords = std::make_shared<textgrid::IntervalTier>("words", 0.0, wordCursor);
        auto tierPhones = std::make_shared<textgrid::IntervalTier>("phones", 0.0, wordCursor);

        for (const auto &item : out) {
            tierWords->AppendInterval(textgrid::Interval(item.start, item.end, item.text));

            if (item.text == "SP" || item.text == "AP") {
                tierPhones->AppendInterval(textgrid::Interval(item.start, item.end, item.text));
                continue;
            }

            for (const auto &phone : item.phones) {
                tierPhones->AppendInterval(textgrid::Interval(phone.start, phone.end, phone.text));
            }
        }

        outTg.AppendTier(tierWords);
        outTg.AppendTier(tierPhones);

        QFile outFile(m_outTgPath);
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream outTgStream(&outFile);
            std::ostringstream oss;
            oss << outTg;
            outTgStream << QString::fromStdString(oss.str());
        } else {
            fblMsg = "Failed to open file for writing.";
        }

        fblMsg = "success.";

        Q_EMIT this->oneFinished(m_filename, QString::fromStdString(fblMsg));
    }
}