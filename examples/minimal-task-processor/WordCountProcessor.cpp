#include "WordCountProcessor.h"

#include <dsfw/IFileIOProvider.h>
#include <dsfw/LocalFileIOProvider.h>
#include <dsfw/ServiceLocator.h>
#include <dsfw/Log.h>

#include <QRegularExpression>
#include <QTextStream>

namespace examples {

static const dstools::TaskProcessorRegistry::Registrar<WordCountProcessor>
    s_registrar("text_analysis", "word-count");

QString WordCountProcessor::processorId() const {
    return "word-count";
}

QString WordCountProcessor::displayName() const {
    return "Word Counter";
}

dstools::TaskSpec WordCountProcessor::taskSpec() const {
    return dstools::TaskSpec{
        "text_analysis",
        {{"text", "input"}},
        {{"statistics", "output"}}
    };
}

dstools::ProcessorConfig WordCountProcessor::capabilities() const {
    return dstools::ProcessorConfig{
        {"caseSensitive", false},
        {"minWordLength", 1}
    };
}

dstools::Result<void> WordCountProcessor::initialize(
    dstools::IModelManager &,
    const dstools::ProcessorConfig &) {
    DSFW_LOG_INFO("example", "WordCountProcessor initialized (no model required)");
    return dstools::Ok();
}

void WordCountProcessor::release() {
    DSFW_LOG_INFO("example", "WordCountProcessor released");
}

dstools::Result<dstools::TaskOutput> WordCountProcessor::process(
    const dstools::TaskInput &input) {
    QString textContent;

    auto it = input.layers.find("text");
    if (it != input.layers.end() && it->second.is_string()) {
        textContent = QString::fromStdString(it->second.get<std::string>());
    } else if (!input.audioPath.isEmpty()) {
        auto *io = dstools::ServiceLocator::fileIO();
        if (!io) {
            return dstools::Err<dstools::TaskOutput>("No IFileIOProvider registered");
        }
        auto result = io->readText(input.audioPath);
        if (!result) {
            return dstools::Err<dstools::TaskOutput>(result.error());
        }
        textContent = result.value();
    } else {
        return dstools::Err<dstools::TaskOutput>("No text input provided");
    }

    QStringList words = textContent.split(QRegularExpression("\\s+"),
                                          Qt::SkipEmptyParts);
    int wordCount = words.size();
    int charCount = textContent.size();
    int lineCount = textContent.count('\n') + (textContent.isEmpty() ? 0 : 1);

    DSFW_LOG_INFO("example",
                   ("WordCount: " + std::to_string(wordCount) + " words, " +
                    std::to_string(charCount) + " chars, " +
                    std::to_string(lineCount) + " lines").c_str());

    dstools::TaskOutput output;
    output.layers["statistics"] = dstools::ProcessorConfig{
        {"wordCount", wordCount},
        {"charCount", charCount},
        {"lineCount", lineCount},
        {"source", input.audioPath.toStdString()}
    };
    return dstools::Result<dstools::TaskOutput>::Ok(std::move(output));
}

} // namespace examples
