#include <dstools/PinyinG2PProvider.h>

#ifdef HAS_CPP_PINYIN
#include <cpp-pinyin/Pinyin.h>
#include <cpp-pinyin/Jyutping.h>
#endif

namespace dstools {

struct PinyinG2PProvider::Impl {
#ifdef HAS_CPP_PINYIN
    Pinyin::Pinyin mandarin;
    Pinyin::Jyutping cantonese;
    bool mandarinOk = false;
    bool cantoneseOk = false;
#endif

    Impl() {
#ifdef HAS_CPP_PINYIN
        mandarinOk = mandarin.initialized();
        cantoneseOk = cantonese.initialized();
#endif
    }
};

PinyinG2PProvider::PinyinG2PProvider()
    : m_impl(std::make_unique<Impl>()) {
}

PinyinG2PProvider::~PinyinG2PProvider() = default;

QString PinyinG2PProvider::providerId() const {
    return QStringLiteral("cpp-pinyin");
}

QStringList PinyinG2PProvider::supportedLanguages() const {
    QStringList langs;
#ifdef HAS_CPP_PINYIN
    if (m_impl->mandarinOk)
        langs << QStringLiteral("zh");
    if (m_impl->cantoneseOk)
        langs << QStringLiteral("yue");
#endif
    return langs;
}

bool PinyinG2PProvider::supportsLanguage(const QString &langCode) const {
    return supportedLanguages().contains(langCode);
}

std::vector<G2PResult> PinyinG2PProvider::convert(const QString &text, const QString &langCode,
                                                   std::string &error) const {
#ifdef HAS_CPP_PINYIN
    if (!supportsLanguage(langCode)) {
        error = "Unsupported language: " + langCode.toStdString();
        return {};
    }

    const std::string input = text.toUtf8().toStdString();
    std::vector<G2PResult> results;

    if (langCode == QStringLiteral("zh")) {
        auto pinyinRes = m_impl->mandarin.hanziToPinyin(input);
        for (const auto &res : pinyinRes) {
            G2PResult r;
            r.word = QString::fromStdString(res.hanzi);
            r.phonemes << QString::fromStdString(res.pinyin);
            r.language = langCode;
            results.push_back(std::move(r));
        }
    } else if (langCode == QStringLiteral("yue")) {
        auto pinyinRes = m_impl->cantonese.hanziToPinyin(input);
        for (const auto &res : pinyinRes) {
            G2PResult r;
            r.word = QString::fromStdString(res.hanzi);
            r.phonemes << QString::fromStdString(res.pinyin);
            r.language = langCode;
            results.push_back(std::move(r));
        }
    }

    return results;
#else
    error = "cpp-pinyin library not available";
    return {};
#endif
}

G2PResult PinyinG2PProvider::convertWord(const QString &word, const QString &langCode,
                                          std::string &error) const {
#ifdef HAS_CPP_PINYIN
    if (!supportsLanguage(langCode)) {
        error = "Unsupported language: " + langCode.toStdString();
        return {};
    }

    const std::string input = word.toUtf8().toStdString();
    G2PResult result;
    result.word = word;
    result.language = langCode;

    if (langCode == QStringLiteral("zh")) {
        auto pinyinVec = m_impl->mandarin.getDefaultPinyin(input);
        for (const auto &p : pinyinVec) {
            result.phonemes << QString::fromStdString(p);
        }
    } else if (langCode == QStringLiteral("yue")) {
        auto pinyinVec = m_impl->cantonese.getDefaultPinyin(input);
        for (const auto &p : pinyinVec) {
            result.phonemes << QString::fromStdString(p);
        }
    }

    return result;
#else
    error = "cpp-pinyin library not available";
    return {};
#endif
}

} // namespace dstools
