#include <iostream>
#include <string>

#include <dsfw/Log.h>
#include <dsfw/PathUtils.h>
#include <dstools/TaskProcessorRegistry.h>
#include <dstools/DsKeys.h>
#include <dstools/ModelManager.h>

#include <syscmdline/parser.h>
#include <syscmdline/command.h>

#include <InferBridge.h>
#include <dsfw/audio/AudioPipeline.h>
#include <dsfw/audio/AudioFileWriter.h>
#include <nlohmann/json.hpp>

#include <fstream>

#include "SlicerService.h"

using namespace SysCmdLine;

static dstools::SlicerService slicerService;

static int cmdSlice(const ParseResult &result) {
    QString input = QString::fromStdString(result.value("input").toString());
    QString outDir = QString::fromStdString(result.valueForOption("out-dir").toString());
    double threshold = result.valueForOption("threshold").toDouble();
    int minLength = result.valueForOption("min-length").toInt();
    int minInterval = result.valueForOption("min-interval").toInt();
    int hopSize = result.valueForOption("hop-size").toInt();
    int maxSilKept = result.valueForOption("max-sil-kept").toInt();

    auto *service = &slicerService;
    auto sliceResult = service->slice(input, threshold, minLength, minInterval, hopSize, maxSilKept);
    if (!sliceResult) {
        std::cerr << "Error: " << sliceResult.error() << std::endl;
        return 1;
    }

    std::cout << "Sliced " << input.toStdString() << " into "
              << sliceResult->chunks.size() << " chunks" << std::endl;
    for (size_t i = 0; i < sliceResult->chunks.size(); ++i) {
        const auto &chunk = sliceResult->chunks[i];
        std::cout << "  Chunk " << i << ": [" << chunk.first << ", " << chunk.second
                  << "] samples" << std::endl;
    }
    return 0;
}

static int cmdAsr(const ParseResult &result) {
    QString input = QString::fromStdString(result.value("input").toString());
    QString modelPath = QString::fromStdString(result.valueForOption("model").toString());

    auto processor = dstools::TaskProcessorRegistry::instance().create(
        QStringLiteral("asr"), QStringLiteral("funasr"));
    if (!processor) {
        std::cerr << "Error: ASR processor not registered" << std::endl;
        return 1;
    }

    dstools::ModelManager mm;
    dstools::ProcessorConfig config;
    config["path"] = modelPath;
    auto initResult = processor->initialize(mm, config);
    if (!initResult) {
        std::cerr << "Error: " << initResult.error() << std::endl;
        return 1;
    }

    dstools::TaskInput taskInput;
    taskInput.audioPath = input;
    auto processResult = processor->process(taskInput);
    if (!processResult) {
        std::cerr << "Error: " << processResult.error() << std::endl;
        return 1;
    }

    if (processResult->layers.count(QStringLiteral("text"))) {
        auto &textLayer = processResult->layers.at(QStringLiteral("text"));
        if (textLayer.toJson().contains("text")) {
            std::cout << textLayer.toJson()["text"].get<std::string>() << std::endl;
        }
    }
    return 0;
}

static int cmdAlign(const ParseResult &result) {
    QString input = QString::fromStdString(result.value("input").toString());
    QString modelPath = QString::fromStdString(result.valueForOption("model").toString());

    auto processor = dstools::TaskProcessorRegistry::instance().create(
        QStringLiteral("phoneme_alignment"), QStringLiteral("hubert-fa"));
    if (!processor) {
        std::cerr << "Error: alignment processor not registered" << std::endl;
        return 1;
    }

    dstools::ModelManager mm;
    dstools::ProcessorConfig config;
    config["path"] = modelPath;
    auto initResult = processor->initialize(mm, config);
    if (!initResult) {
        std::cerr << "Error: " << initResult.error() << std::endl;
        return 1;
    }

    dstools::TaskInput taskInput;
    taskInput.audioPath = input;
    auto processResult = processor->process(taskInput);
    if (!processResult) {
        std::cerr << "Error: " << processResult.error() << std::endl;
        return 1;
    }

    if (processResult->layers.count(QString::fromUtf8(dstools::keys::layers::phoneme))) {
        auto &layer = processResult->layers.at(QString::fromUtf8(dstools::keys::layers::phoneme));
        std::cout << layer.toJson().dump(2) << std::endl;
    }
    return 0;
}

static int cmdPitch(const ParseResult &result) {
    QString input = QString::fromStdString(result.value("input").toString());
    QString modelPath = QString::fromStdString(result.valueForOption("model").toString());

    auto processor = dstools::TaskProcessorRegistry::instance().create(
        QStringLiteral("pitch_extraction"), QStringLiteral("rmvpe"));
    if (!processor) {
        std::cerr << "Error: pitch processor not registered" << std::endl;
        return 1;
    }

    dstools::ModelManager mm;
    dstools::ProcessorConfig config;
    config["path"] = modelPath;
    auto initResult = processor->initialize(mm, config);
    if (!initResult) {
        std::cerr << "Error: " << initResult.error() << std::endl;
        return 1;
    }

    dstools::TaskInput taskInput;
    taskInput.audioPath = input;
    auto processResult = processor->process(taskInput);
    if (!processResult) {
        std::cerr << "Error: " << processResult.error() << std::endl;
        return 1;
    }

    if (processResult->layers.count(QString::fromUtf8(dstools::keys::layers::pitch))) {
        auto &layer = processResult->layers.at(QString::fromUtf8(dstools::keys::layers::pitch));
        std::cout << layer.toJson().dump(2) << std::endl;
    }
    return 0;
}

static int cmdTranscribe(const ParseResult &result) {
    QString input = QString::fromStdString(result.value("input").toString());
    QString modelPath = QString::fromStdString(result.valueForOption("model").toString());

    auto processor = dstools::TaskProcessorRegistry::instance().create(
        QStringLiteral("midi_transcription"), QStringLiteral("game"));
    if (!processor) {
        std::cerr << "Error: transcription processor not registered" << std::endl;
        return 1;
    }

    dstools::ModelManager mm;
    dstools::ProcessorConfig config;
    config["path"] = modelPath;
    auto initResult = processor->initialize(mm, config);
    if (!initResult) {
        std::cerr << "Error: " << initResult.error() << std::endl;
        return 1;
    }

    dstools::TaskInput taskInput;
    taskInput.audioPath = input;
    auto processResult = processor->process(taskInput);
    if (!processResult) {
        std::cerr << "Error: " << processResult.error() << std::endl;
        return 1;
    }

    if (processResult->layers.count(QString::fromUtf8(dstools::keys::layers::midi))) {
        auto &layer = processResult->layers.at(QString::fromUtf8(dstools::keys::layers::midi));
        std::cout << layer.toJson().dump(2) << std::endl;
    }
    return 0;
}

static int cmdHfa(const ParseResult &result) {
    QString modelPath = QString::fromStdString(result.valueForOption("model").toString());
    QString wavPath = QString::fromStdString(result.valueForOption("wav").toString());
    QString labPath = QString::fromStdString(result.valueForOption("lab").toString());
    QString saveWavPath = QString::fromStdString(result.valueForOption("save-wav").toString());

    if (modelPath.isEmpty() || wavPath.isEmpty() || labPath.isEmpty()) {
        std::cerr << "Error: --model, --wav, and --lab are required" << std::endl;
        return 1;
    }

    auto modelStdPath = dsfw::PathUtils::toStdPath(modelPath);
    auto wavStdPath = dsfw::PathUtils::toStdPath(wavPath);

    int modelSampleRate = 0;
    {
        auto cfgPath = modelStdPath / "config.json";
        std::ifstream f(cfgPath);
        if (f.is_open()) {
            auto cfg = nlohmann::json::parse(f);
            modelSampleRate = cfg["mel_spec_config"]["sample_rate"].get<int>();
        }
    }

    if (!saveWavPath.isEmpty() && modelSampleRate > 0) {
        auto pipeline = dsfw::audio::AudioPipeline::create();
        auto result = pipeline.decodeToMonoFloat(dsfw::PathUtils::toUtf8(wavStdPath), modelSampleRate);
        if (!result.ok()) {
            std::cerr << "Decode error: " << result.error() << std::endl;
            return 1;
        }
        auto buffer = result.value();
        auto floats = buffer.floats();

        const auto saveStdPath = dsfw::PathUtils::toStdPath(saveWavPath);
        dsfw::audio::WriteConfig wcfg;
        wcfg.sampleRate = modelSampleRate;
        wcfg.channelCount = 1;
        wcfg.format = dsfw::audio::SampleFormat::Int16;
        dsfw::audio::AudioFileWriter writer;
        auto openResult = writer.open(dsfw::PathUtils::toUtf8(saveStdPath), wcfg);
        if (!openResult.ok()) {
            std::cerr << "Failed to write WAV: " << openResult.error() << std::endl;
            return 1;
        }
        // Convert float32 to int16
        std::vector<int16_t> int16Samples(floats.size());
        for (size_t i = 0; i < floats.size(); ++i) {
            const float clamped = std::max(-1.0f, std::min(1.0f, floats[i]));
            int16Samples[i] = static_cast<int16_t>(clamped * 32767.0f);
        }
        writer.writeInt16s(int16Samples.data(), static_cast<int64_t>(int16Samples.size()));
        writer.close();
    }

    std::string lyrics;
    {
        auto labStdPath = dsfw::PathUtils::toStdPath(labPath);
        std::ifstream f(labStdPath);
        if (!f.is_open()) {
            std::cerr << "Error: cannot open " << dsfw::PathUtils::toUtf8(labStdPath) << std::endl;
            return 1;
        }
        lyrics = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }
    if (!lyrics.empty() && lyrics.back() == '\n') {
        lyrics.pop_back();
    }

    HFA::HFA hfa;
    auto loadResult = hfa.load(modelStdPath, HFA::ExecutionProvider::CPU, -1);
    if (!loadResult) {
        std::cerr << "Failed to load model: " << loadResult.error() << std::endl;
        return 1;
    }

    std::vector<std::string> nonSpeechPh = {"AP", "SP"};
    HFA::WordList words;
    auto hfaResult = hfa.recognize(wavStdPath, "zh", nonSpeechPh, lyrics, words);
    if (!hfaResult) {
        std::cerr << "FA failed: " << hfaResult.error() << std::endl;
        return 1;
    }

    nlohmann::json j;
    j["words"] = nlohmann::json::array();
    j["phonemes"] = nlohmann::json::array();

    for (const auto &word : words) {
        nlohmann::json wj;
        wj["text"] = word.text;
        wj["start"] = word.start;
        wj["end"] = word.end;
        wj["phones"] = nlohmann::json::array();
        for (const auto &ph : word.phones) {
            nlohmann::json pj;
            pj["phone"] = ph.text;
            pj["start"] = ph.start;
            pj["end"] = ph.end;
            wj["phones"].push_back(pj);
            j["phonemes"].push_back(pj);
        }
        j["words"].push_back(std::move(wj));
    }

    std::cout << j.dump(2) << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    dstools::Logger::instance().setMinLevel(dstools::LogLevel::Warning);

    Command rootCommand("dstools", "DiffSinger dataset processing CLI");

    Command sliceCommand("slice", "Slice audio file by silence detection");
    sliceCommand.addArgument(Argument("input", "Input audio file path"));
    sliceCommand.addOption(Option(std::string("out-dir"), "Output directory",
                                  Argument("path", "Output path", false)));
    sliceCommand.addOption(Option(std::string("threshold"), "Silence threshold in dB",
                                  Argument("dB", "Threshold value", false, Value("-40"))));
    sliceCommand.addOption(Option(std::string("min-length"), "Minimum chunk length in ms",
                                  Argument("ms", "Length", false, Value("5000"))));
    sliceCommand.addOption(Option(std::string("min-interval"), "Minimum interval in ms",
                                  Argument("ms", "Interval", false, Value("300"))));
    sliceCommand.addOption(Option(std::string("hop-size"), "Hop size in ms",
                                  Argument("ms", "Hop", false, Value("20"))));
    sliceCommand.addOption(Option(std::string("max-sil-kept"), "Max silence kept at edges in ms",
                                  Argument("ms", "Duration", false, Value("5000"))));
    sliceCommand.setHandler(cmdSlice);

    Command asrCommand("asr", "Speech recognition on audio file");
    asrCommand.addArgument(Argument("input", "Input audio file path"));
    asrCommand.setHandler(cmdAsr);

    Command alignCommand("align", "Phoneme alignment on audio file");
    alignCommand.addArgument(Argument("input", "Input audio file path"));
    alignCommand.setHandler(cmdAlign);

    Command pitchCommand("pitch", "Extract pitch (F0) from audio file");
    pitchCommand.addArgument(Argument("input", "Input audio file path"));
    pitchCommand.setHandler(cmdPitch);

    Command transcribeCommand("transcribe", "Transcribe notes from audio file");
    transcribeCommand.addArgument(Argument("input", "Input audio file path"));
    transcribeCommand.setHandler(cmdTranscribe);

    Command hfaCommand("hfa", "Hubert Forced Aligner on audio file");
    hfaCommand.addOption(Option(std::string("model"), "Model folder path",
                                Argument("path", "Model path", true)));
    hfaCommand.addOption(Option(std::string("wav"), "Input audio file path",
                                Argument("path", "Audio path", true)));
    hfaCommand.addOption(Option(std::string("lab"), "Input lyrics .lab file path",
                                Argument("path", "Lab path", true)));
    hfaCommand.addOption(Option(std::string("save-wav"), "Resampled output audio path",
                                Argument("path", "Output path", false)));
    hfaCommand.setHandler(cmdHfa);

    rootCommand.addCommand(sliceCommand);
    rootCommand.addCommand(asrCommand);
    rootCommand.addCommand(alignCommand);
    rootCommand.addCommand(pitchCommand);
    rootCommand.addCommand(transcribeCommand);
    rootCommand.addCommand(hfaCommand);
    rootCommand.addHelpOption(true);

    Parser parser(rootCommand);
    return parser.invoke(argc, argv);
}
