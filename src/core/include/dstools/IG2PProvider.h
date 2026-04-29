#pragma once

#include <QString>
#include <QStringList>

#include <string>
#include <vector>

namespace dstools {

struct G2PResult {
    QString word;
    QStringList phonemes;
    QString language;
};

class IG2PProvider {
public:
    virtual ~IG2PProvider() = default;
    virtual QString providerId() const = 0;
    virtual QStringList supportedLanguages() const = 0;
    virtual bool supportsLanguage(const QString &langCode) const = 0;
    virtual std::vector<G2PResult> convert(const QString &text, const QString &langCode, std::string &error) const = 0;
    virtual G2PResult convertWord(const QString &word, const QString &langCode, std::string &error) const = 0;
};

class StubG2PProvider : public IG2PProvider {
public:
    QString providerId() const override { return "stub"; }
    QStringList supportedLanguages() const override { return {}; }
    bool supportsLanguage(const QString &) const override { return false; }
    std::vector<G2PResult> convert(const QString &, const QString &, std::string &error) const override {
        error = "G2P provider not implemented";
        return {};
    }
    G2PResult convertWord(const QString &, const QString &, std::string &error) const override {
        error = "G2P provider not implemented";
        return {};
    }
};

} // namespace dstools
