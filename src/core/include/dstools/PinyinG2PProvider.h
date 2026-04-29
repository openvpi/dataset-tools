#pragma once

#include <memory>

#include <dstools/IG2PProvider.h>

namespace dstools {

class PinyinG2PProvider : public IG2PProvider {
public:
    PinyinG2PProvider();
    ~PinyinG2PProvider() override;

    QString providerId() const override;
    QStringList supportedLanguages() const override;
    bool supportsLanguage(const QString &langCode) const override;
    std::vector<G2PResult> convert(const QString &text, const QString &langCode, std::string &error) const override;
    G2PResult convertWord(const QString &word, const QString &langCode, std::string &error) const override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace dstools
