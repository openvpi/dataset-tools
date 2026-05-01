#include <QCoreApplication>

#include <dsfw/AppSettings.h>
#include <dsfw/LocalFileIOProvider.h>
#include <dsfw/Log.h>
#include <dsfw/ServiceLocator.h>
#include <dsfw/TaskProcessorRegistry.h>
#include <dsfw/TaskTypes.h>

#include "WordCountProcessor.h"

namespace AppKeys {
inline const dstools::SettingsKey<QString> LastInputPath("General/lastInputPath", "");
inline const dstools::SettingsKey<bool> CaseSensitive("Analysis/caseSensitive", false);
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("minimal-task-processor");

    auto &logger = dstools::Logger::instance();
    logger.addSink([](const dstools::LogEntry &entry) {
        const char *levelStr = "UNKNOWN";
        switch (entry.level) {
        case dstools::LogLevel::Trace: levelStr = "TRACE"; break;
        case dstools::LogLevel::Debug: levelStr = "DEBUG"; break;
        case dstools::LogLevel::Info: levelStr = "INFO"; break;
        case dstools::LogLevel::Warning: levelStr = "WARN"; break;
        case dstools::LogLevel::Error: levelStr = "ERROR"; break;
        case dstools::LogLevel::Fatal: levelStr = "FATAL"; break;
        }
        std::fprintf(stderr, "[%s] [%s] %s\n", levelStr, entry.category.c_str(),
                     entry.message.c_str());
    });
    logger.setMinLevel(dstools::LogLevel::Info);

    DSFW_LOG_INFO("main", "minimal-task-processor starting...");

    dstools::LocalFileIOProvider fileIO;
    dstools::ServiceLocator::setFileIO(&fileIO);
    DSFW_LOG_INFO("main", "IFileIOProvider registered via ServiceLocator");

    dstools::AppSettings settings("minimal-task-processor");
    bool caseSensitive = settings.get(AppKeys::CaseSensitive);
    DSFW_LOG_INFO("main",
                   ("CaseSensitive setting: " + std::string(caseSensitive ? "true" : "false"))
                       .c_str());

    auto &registry = dstools::TaskProcessorRegistry::instance();
    QStringList tasks = registry.availableTasks();
    DSFW_LOG_INFO("main",
                   ("Registered tasks: " + tasks.join(", ").toStdString()).c_str());

    for (const auto &taskName : tasks) {
        QStringList processors = registry.availableProcessors(taskName);
        for (const auto &procId : processors) {
            DSFW_LOG_INFO("registry",
                           ("  Task '" + taskName.toStdString() + "' -> processor '" +
                            procId.toStdString() + "'")
                               .c_str());
        }
    }

    auto processor = registry.create("text_analysis", "word-count");
    if (!processor) {
        DSFW_LOG_ERROR("main", "Failed to create word-count processor");
        return 1;
    }

    DSFW_LOG_INFO("main",
                   ("Created processor: " + processor->displayName().toStdString()).c_str());

    dstools::TaskInput input;
    input.layers["text"] = "Hello dsfw framework!\n"
                           "This is a minimal example demonstrating\n"
                           "the task processor registry pattern.";

    auto result = processor->process(input);
    if (!result) {
        DSFW_LOG_ERROR("main", result.error().c_str());
        return 1;
    }

    const auto &output = result.value();
    auto statsIt = output.layers.find("statistics");
    if (statsIt != output.layers.end()) {
        const auto &stats = statsIt->second;
        DSFW_LOG_INFO("result",
                       ("Word count: " + std::to_string(stats["wordCount"].get<int>())).c_str());
        DSFW_LOG_INFO("result",
                       ("Char count: " + std::to_string(stats["charCount"].get<int>())).c_str());
        DSFW_LOG_INFO("result",
                       ("Line count: " + std::to_string(stats["lineCount"].get<int>())).c_str());
    }

    settings.set(AppKeys::CaseSensitive, true);
    settings.flush();
    DSFW_LOG_INFO("main", "Settings flushed to disk");

    DSFW_LOG_INFO("main", "minimal-task-processor finished.");
    return 0;
}
