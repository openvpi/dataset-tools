#pragma once
/// @file MatchLyric.h
/// @brief Batch lyric matching controller (P-17: file I/O via injectable loader).

#include "LyricAlignment.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QTextStream>
#include <memory>

namespace LyricFA {

    struct LyricDictEntry {
        QVector<QString> text;
        QVector<QString> pinyin;
    };

    /// @brief Abstracts all file I/O for lyric matching. (P-17)
    class ILyricFileLoader {
    public:
        virtual ~ILyricFileLoader() = default;
        virtual QMap<QString, LyricDictEntry> loadDict(const QString &folder, LyricMatcher &matcher) = 0;
        virtual QString readLabContent(const QString &labPath) = 0;
        virtual QString readTextFile(const QString &path) = 0;
        virtual bool writeJsonFile(const QString &path, const QString &text, const QString &phonetic) = 0;
        virtual bool writeLabFile(const QString &path, const QString &content) = 0;
    };

    /// @brief Default QFile/QDir-based loader implementation.
    class DefaultLyricFileLoader : public ILyricFileLoader {
    public:
        QMap<QString, LyricDictEntry> loadDict(const QString &folder, LyricMatcher &matcher) override {
            QMap<QString, LyricDictEntry> dict;
            const QDir dir(folder);
            const QStringList filters{"*.txt"};
            const QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
            for (const QFileInfo &fi : files) {
                const QString name = fi.completeBaseName();
                try {
                    const QString content = readTextFile(fi.absoluteFilePath());
                    const LyricData data = matcher.processLyricContent(content);
                    dict[name] = {data.text_list, data.phonetic_list};
                } catch (const std::exception &e) {
                    qWarning() << "Failed to load lyric file:" << fi.absoluteFilePath() << "-" << e.what();
                }
            }
            return dict;
        }

        QString readLabContent(const QString &labPath) override {
            return readTextFile(labPath);
        }

        QString readTextFile(const QString &path) override {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return {};
            QTextStream stream(&file);
            const QString content = stream.readAll().trimmed();
            file.close();
            return content;
        }

        bool writeJsonFile(const QString &path, const QString &text, const QString &phonetic) override {
            QJsonObject obj;
            obj["raw_text"] = text;
            obj["lab"] = phonetic;
            obj["lab_without_tone"] = phonetic;
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return false;
            file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
            file.close();
            return true;
        }

        bool writeLabFile(const QString &path, const QString &content) override {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return false;
            QTextStream stream(&file);
            stream << content;
            file.close();
            return true;
        }
    };

    /// @brief Manages a dictionary of reference lyrics and matches individual ASR results against them.
    class MatchLyric {
    public:
        explicit MatchLyric(std::unique_ptr<ILyricFileLoader> loader = std::make_unique<DefaultLyricFileLoader>());
        ~MatchLyric();

        /// @brief Load all lyric files from a directory into the internal dictionary.
        /// @param lyric_folder Path to the folder containing lyric files.
        void initLyric(const QString &lyric_folder);

        /// @brief Match a single ASR result against cached lyrics.
        /// @param filename Name of the lyric to match against.
        /// @param labPath Path to the ASR .lab file.
        /// @param jsonPath Output path for the alignment JSON result.
        /// @param msg Output message describing success or failure.
        /// @return True if matching succeeded.
        bool match(const QString &filename, const QString &labPath, const QString &jsonPath, QString &msg) const;

        bool matchText(const QString &filename, const QString &asrText, QString &matchedText, QString &msg) const;

        bool isInitialized() const {
            return !m_lyricDict.isEmpty();
        }

    private:
        /// @brief Cached lyric text and pinyin sequences.
        struct LyricInfo {
            QVector<QString> text;   ///< Lyric text segments.
            QVector<QString> pinyin; ///< Pinyin phonetic sequences.
        };

        QMap<QString, LyricInfo> m_lyricDict;       ///< Dictionary mapping lyric names to cached data.
        std::unique_ptr<LyricMatcher> m_matcher;    ///< Lyric matcher instance.
        std::unique_ptr<ILyricFileLoader> m_loader; ///< Injectable file loader (P-17).
        int m_diffThreshold = 1;                    ///< Maximum allowed difference count for a valid match.
    };
}
