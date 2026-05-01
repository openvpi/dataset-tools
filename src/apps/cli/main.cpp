#include <iostream>
#include <string>

#include <dsfw/Log.h>
#include <dsfw/ISlicerService.h>
#include <dsfw/ServiceLocator.h>
#include <dsfw/TaskProcessorRegistry.h>
#include <dstools/ModelManager.h>

#include <syscmdline/parser.h>
#include <syscmdline/command.h>

#include "SlicerService.h"

using namespace SysCmdLine;

static int cmdSlice(const ParseResult &result) {
    QString input = QString::fromStdString(result.value("input").toString());
    QString outDir = QString::fromStdString(result.valueForOption("out-dir").toString());
    double threshold = result.valueForOption("threshold").toDouble();
    int minLength = result.valueForOption("min-length").toInt();
    int minInterval = result.valueForOption("min-interval").toInt();
    int hopSize = result.valueForOption("hop-size").toInt();

    auto *service = dstools::ServiceLocator::get<dstools::ISlicerService>();
    if (!service) {
        std::cerr << "Error: slicer service not available" << std::endl;
        return 1;
    }

    auto sliceResult = service->slice(input, threshold, minLength, minInterval, hopSize);
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
    config["path"] = modelPath.toStdString();
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
        if (textLayer.contains("text")) {
            std::cout << textLayer["text"].get<std::string>() << std::endl;
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
    config["path"] = modelPath.toStdString();
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

    if (processResult->layers.count(QStringLiteral("phoneme"))) {
        auto &layer = processResult->layers.at(QStringLiteral("phoneme"));
        std::cout << layer.dump(2) << std::endl;
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
    config["path"] = modelPath.toStdString();
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

    if (processResult->layers.count(QStringLiteral("pitch"))) {
        auto &layer = processResult->layers.at(QStringLiteral("pitch"));
        std::cout << layer.dump(2) << std::endl;
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
    config["path"] = modelPath.toStdString();
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

    if (processResult->layers.count(QStringLiteral("midi"))) {
        auto &layer = processResult->layers.at(QStringLiteral("midi"));
        std::cout << layer.dump(2) << std::endl;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    dstools::Logger::instance().setMinLevel(dstools::LogLevel::Warning);

    static SlicerService slicerService;
    dstools::ServiceLocator::set<dstools::ISlicerService>(&slicerService);

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

    rootCommand.addCommand(sliceCommand);
    rootCommand.addCommand(asrCommand);
    rootCommand.addCommand(alignCommand);
    rootCommand.addCommand(pitchCommand);
    rootCommand.addCommand(transcribeCommand);
    rootCommand.addHelpOption(true);

    Parser parser(rootCommand);
    return parser.invoke(argc, argv);
}
