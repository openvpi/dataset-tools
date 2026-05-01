#include <iostream>
#include <string>

#include <dsfw/Log.h>
#include <dsfw/ISlicerService.h>
#include <dsfw/IAsrService.h>
#include <dsfw/IAlignmentService.h>
#include <dsfw/IPitchService.h>
#include <dsfw/ITranscriptionService.h>
#include <dsfw/ServiceLocator.h>

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

    auto *service = dstools::ServiceLocator::get<dstools::IAsrService>();
    if (!service) {
        std::cerr << "Error: ASR service not available" << std::endl;
        return 1;
    }

    auto asrResult = service->recognize(input);
    if (!asrResult) {
        std::cerr << "Error: " << asrResult.error() << std::endl;
        return 1;
    }

    std::cout << asrResult->text.toStdString() << std::endl;
    return 0;
}

static int cmdAlign(const ParseResult &result) {
    QString input = QString::fromStdString(result.value("input").toString());

    auto *service = dstools::ServiceLocator::get<dstools::IAlignmentService>();
    if (!service) {
        std::cerr << "Error: alignment service not available" << std::endl;
        return 1;
    }

    // Align requires phonemes but CLI input is simplified
    std::vector<QString> phonemes;
    auto alignResult = service->align(input, phonemes);
    if (!alignResult) {
        std::cerr << "Error: " << alignResult.error() << std::endl;
        return 1;
    }

    for (const auto &phone : alignResult->phones) {
        std::cout << phone.phone.toStdString() << "\t"
                  << phone.startSec << "\t" << phone.endSec << std::endl;
    }
    return 0;
}

static int cmdPitch(const ParseResult &result) {
    QString input = QString::fromStdString(result.value("input").toString());

    auto *service = dstools::ServiceLocator::get<dstools::IPitchService>();
    if (!service) {
        std::cerr << "Error: pitch service not available" << std::endl;
        return 1;
    }

    auto pitchResult = service->extractPitch(input);
    if (!pitchResult) {
        std::cerr << "Error: " << pitchResult.error() << std::endl;
        return 1;
    }

    std::cout << "Extracted " << pitchResult->f0.size() << " pitch frames"
              << " (hop=" << pitchResult->hopMs << "ms)" << std::endl;
    return 0;
}

static int cmdTranscribe(const ParseResult &result) {
    QString input = QString::fromStdString(result.value("input").toString());

    auto *service = dstools::ServiceLocator::get<dstools::ITranscriptionService>();
    if (!service) {
        std::cerr << "Error: transcription service not available" << std::endl;
        return 1;
    }

    auto transResult = service->transcribe(input);
    if (!transResult) {
        std::cerr << "Error: " << transResult.error() << std::endl;
        return 1;
    }

    for (const auto &note : transResult->notes) {
        std::cout << "Note: pitch=" << note.pitch
                  << " start=" << note.startFrame
                  << " end=" << note.endFrame
                  << " vel=" << note.velocity << std::endl;
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
