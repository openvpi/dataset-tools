#include "Aligner.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include <audio-util/Util.h>
#include <sndfile.hh>

#include "HostDecoding.h"
#include "HostScoring.h"
#include "PathUtils.h"
#include "TextUtils.h"

namespace Tifa {

    namespace {

        /** Models above this length need an impractical amount of fragment memory. */
        constexpr int64_t kVeryLongUtteranceSeconds = 600;

    } // namespace

    bool Aligner::load(const std::filesystem::path &modelDir, const std::filesystem::path &dictDir,
                    const ExecutionProvider provider, const int deviceId, std::string &error) {
        m_initialized = false;

        if (!m_config.load(modelDir / "config.json", error)) {
            return false;
        }
        if (!m_vocabulary.load(modelDir / "vocabulary.json", error)) {
            return false;
        }
        if (!m_model.load(modelDir, provider, deviceId, error)) {
            return false;
        }

        G2PResources resources;
        resources.dictDir = dictDir;
        resources.pinyinDir = dictDir;
        if (!m_g2p.init(resources, error)) {
            return false;
        }

        m_initialized = true;
        return true;
    }

    bool Aligner::loadAudio(const std::filesystem::path &path, std::vector<float> &samples, std::string &error) const {
        std::string message;
        auto sf_vio = AudioUtil::resample_to_vio(path, message, 1, m_config.sampleRate);
        if (sf_vio.data.byteArray.empty()) {
            error = "Failed to read '" + pathToUtf8(path) + "': " + message;
            return false;
        }

        SndfileHandle handle(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1,
                             m_config.sampleRate);
        const auto frames = handle.frames();
        if (frames <= 0) {
            error = "The audio contains no samples: " + pathToUtf8(path);
            return false;
        }
        if (handle.channels() != 1) {
            error = "The audio is not mono: " + pathToUtf8(path);
            return false;
        }

        samples.resize(static_cast<size_t>(frames));
        handle.seek(0, SEEK_SET);
        if (handle.read(samples.data(), static_cast<sf_count_t>(samples.size())) != frames) {
            error = "Failed to decode the audio samples: " + pathToUtf8(path);
            return false;
        }
        return true;
    }

    bool Aligner::align(const AlignRequest &request, AlignResult &result, std::string &error) const {
        if (!m_initialized) {
            error = "The aligner is not loaded";
            return false;
        }

        const std::lock_guard<std::mutex> lock(m_mutex);
        result = AlignResult{};

        // Only the selected language may claim the text: a second tag would also
        // activate the converters of that language, which read hanzi differently.
        const std::vector<std::string> languages = {languageTag(request.language)};
        const std::u32string text = Text::decode(request.text);
        if (Text::splitWords(text).empty()) {
            error = "The text of the file is empty";
            return false;
        }

        // ---- G2P and candidate encoding ----
        std::vector<G2PWord> words;
        std::string g2pError;
        if (!m_g2p.convert(text, languages, words, g2pError)) {
            error = g2pError;
            return false;
        }
        if (words.empty()) {
            error = "G2P produced no words";
            return false;
        }

        int64_t readings = 0;
        for (const G2PWord &word : words) {
            for (const G2PReading &reading : word.readings) {
                readings += static_cast<int64_t>(reading.paths.size());
            }
        }

        PathGrid grid;
        if (!encodePaths(words, m_vocabulary, request.oovHandling, languages, grid, error)) {
            return false;
        }

        int64_t candidates = 0;
        for (const std::vector<CandidateInfo> &lexicon : grid.lexicon) {
            candidates += static_cast<int64_t>(lexicon.size());
        }
        if (candidates < readings) {
            result.warnings.push_back("Dropped " + std::to_string(readings - candidates) +
                                      " pronunciation(s) that the vocabulary does not know");
        }
        // TIFA skips a file whose grid holds no token at all; an empty
        // pronunciation of an otherwise valid word lands here too.
        const bool anyToken = std::any_of(grid.tokens.begin(), grid.tokens.end(),
                                          [](const int64_t token) { return token != 0; });
        if (candidates == 0 || !anyToken) {
            error = "No valid token sequence";
            return false;
        }

        // ---- Audio and spectrogram ----
        std::vector<float> samples;
        if (!loadAudio(request.wavPath, samples, error)) {
            return false;
        }
        const double duration = static_cast<double>(samples.size()) / m_config.sampleRate;
        if (duration > kVeryLongUtteranceSeconds) {
            result.warnings.push_back("The audio is longer than " + std::to_string(kVeryLongUtteranceSeconds) +
                                      " seconds; aligning it needs a large amount of memory");
        }

        SpectrogramOutput spectrogram;
        if (!m_model.spectrogram(samples, static_cast<float>(duration), spectrogram, error)) {
            return false;
        }
        int64_t frames = 0;
        for (const uint8_t value : spectrogram.mask) {
            frames += value != 0 ? 1 : 0;
        }
        if (frames == 0) {
            error = "The audio is too short to align";
            return false;
        }

        // ---- Candidate scoring ----
        std::vector<int64_t> prepared;
        std::vector<int64_t> segments;
        std::vector<int64_t> mapping;
        if (!m_model.prepare(grid, prepared, segments, mapping, error)) {
            return false;
        }

        std::vector<int64_t> choices;
        firstChoices(grid, choices);
        const bool scoring = std::any_of(segments.begin(), segments.end(), [](const int64_t value) {
            return value > 0;
        });
        if (scoring) {
            FeatureOutput features;
            if (!m_model.features(spectrogram, prepared.data(), grid.capacity, features, error)) {
                return false;
            }
            ScoreOutput scored;
            if (!m_model.score(features.logits, grid.capacity, features.vocab, grid, segments, mapping, scored,
                               error)) {
                return false;
            }
            CandidateScores scores;
            if (!selectScoredPaths(grid, scored, features.vocab, scores, error)) {
                return false;
            }
            choices = std::move(scores.choices);
        }

        // ---- Materialize the chosen candidates ----
        SelectOutput selected;
        if (!m_model.select(grid, choices, selected, error)) {
            return false;
        }
        int64_t phones = 0;
        for (const uint8_t value : selected.mask) {
            phones += value != 0 ? 1 : 0;
        }

        Alignment alignment;
        alignment.timestep = m_config.timestep;
        alignment.frames = frames;
        alignment.texts = grid.texts;
        for (int64_t w = 0; w < grid.wordCount; ++w) {
            const int64_t choice = choices[static_cast<size_t>(w)];
            if (choice <= 0) {
                continue;
            }
            const CandidateInfo &candidate = grid.lexicon[static_cast<size_t>(w)][static_cast<size_t>(choice) - 1];
            alignment.phonemes.insert(alignment.phonemes.end(), candidate.phonemes.begin(), candidate.phonemes.end());
            alignment.scripts.insert(alignment.scripts.end(), candidate.scripts.begin(), candidate.scripts.end());
        }

        if (phones > 0) {
            FeatureOutput features;
            if (!m_model.features(spectrogram, selected.tokens.data(), selected.capacity, features, error)) {
                return false;
            }

            std::vector<int64_t> groups(selected.groups.begin(), selected.groups.begin() + phones);
            std::vector<int64_t> spans;
            if (!decodeAlignment(features.similarities.data(), selected.capacity, frames, phones, groups,
                                 request.skipPenalty, spans, error)) {
                return false;
            }
            alignment.spans = std::move(spans);
            alignment.words.assign(selected.words.begin(), selected.words.begin() + phones);
            alignment.groups = std::move(groups);
            result.agreement = computeAgreement(features.logits, features.vocab, selected.tokens.data(), phones);
        }

        if (static_cast<int64_t>(alignment.phonemes.size()) != phones) {
            error = "The materialized tokens do not match the chosen pronunciations";
            return false;
        }

        // ---- TextGrid ----
        const TextGridResult textGrid = buildTextGrid(alignment, request.skipHandling, languageTag(request.language));
        result.textGrid = textGrid.content;
        result.report = textGrid.report;
        result.phones = phones;
        result.words = grid.wordCount;
        for (int64_t i = 0; i < phones; ++i) {
            if (alignment.spans[static_cast<size_t>(i) * 2] >= alignment.spans[static_cast<size_t>(i) * 2 + 1]) {
                result.skipped += 1;
            }
        }
        return true;
    }

} // namespace Tifa
