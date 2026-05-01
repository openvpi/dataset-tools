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

using namespace SysCmdLine;

static int cmdSlice(const ParseResult &result) {
    QString input = QString::fromStdString(result.valueForArg("input").toString());
    QString outDir = QString::fromStdString(result.valueForArg("out-dir").toString());
    double threshold = result.valueForArg("threshold").toDouble();
    int minLength = result.valueForArg("min-length").toInt();
    int minInterval = result.valueForArg("min-interval").toInt();
    int hopSize = result.valueForArg("hop-size").toInt();

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
    QString input = QString::fromStdString(result.valueForArg("input").toString());
    QString model = QString::fromStdString(result.valueForArg("model").toString());

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
    QString input = QString::fromStdString(result.valueForArg("input").toString());
    std::vector<QString> phonemes;
    for (const auto &p : result.valuesForArg("phonemes")) {
        phonemes.push_back(QString::fromStdString(p.toString()));
    }

    auto *service = dstools::ServiceLocator::get<dstools::IAlignmentService>();
    if (!service) {
        std::cerr << "Error: alignment service not available" << std::endl;
        return 1;
    }

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
    QString input = QString::fromStdString(result.valueForArg("input").toString());

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
    QString input = QString::fromStdString(result.valueForArg("input").toString());

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
    dstools::Log::setLevel(dstools::Log::Warning);

    Command sliceCommand("slice");
    sliceCommand.setDescription("Slice audio file by silence detection");
    sliceCommand.addArgument(Argument("input", "Input audio file path"));
    sliceCommand.addOption(Option("out-dir", {"/path/to/output"}, "Output directory"));
    sliceCommand.addOption(Option("threshold", {"-40"}, "Silence threshold in dB", "t"));
    sliceCommand.addOption(Option("min-length", {"5000"}, "Minimum chunk length in ms"));
    sliceCommand.addOption(Option("min-interval", {"300"}, "Minimum interval in ms"));
    sliceCommand.addOption(Option("hop-size", {"20"}, "Hop size in ms"));
    sliceCommand.setHandler(cmdSlice);

    Command asrCommand("asr");
    asrCommand.setDescription("Speech recognition on audio file");
    asrCommand.addArgument(Argument("input", "Input audio file path"));
    asrCommand.addOption(Option("model", {"/path/to/model"}, "ASR model path", "m"));
    asrCommand.setHandler(cmdAsr);

    Command alignCommand("align");
    alignCommand.setDescription("Phoneme alignment on audio file");
    alignCommand.addArgument(Argument("input", "Input audio file path"));
    alignCommand.addArgument(Argument("phonemes", "Phoneme sequence").setMultiValue(true));
    alignCommand.setHandler(cmdAlign);

    Command pitchCommand("pitch");
    pitchCommand.setDescription("Extract pitch (F0) from audio file");
    pitchCommand.addArgument(Argument("input", "Input audio file path"));
    pitchCommand.setHandler(cmdPitch);

    Command transcribeCommand("transcribe");
    transcribeCommand.setDescription("Transcribe notes from audio file");
    transcribeCommand.addArgument(Argument("input", "Input audio file path"));
    transcribeCommand.setHandler(cmdTranscribe);

    Parser parser;
    parser.addCommand(sliceCommand);
    parser.addCommand(asrCommand);
    parser.addCommand(alignCommand);
    parser.addCommand(pitchCommand);
    parser.addCommand(transcribeCommand);
    parser.setMainCommand("dstools");

    return parser.invoke(argc, argv);
}
