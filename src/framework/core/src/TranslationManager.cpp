#include <dsfw/TranslationManager.h>

#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QTranslator>

namespace dsfw {

QString TranslationManager::s_currentLanguage;

static QStringList defaultSearchPaths() {
    return {
        QStringLiteral(":/translations"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/translations"),
    };
}

void TranslationManager::install(const QString &language, const QStringList &searchPaths) {
    const QStringList paths = searchPaths.isEmpty() ? defaultSearchPaths() : searchPaths;
    const QString locale = language.isEmpty() ? QLocale::system().name() : language;

    for (const auto &dir : paths) {
        QDir d(dir);
        if (!d.exists())
            continue;

        const auto entries = d.entryList({QStringLiteral("*_%1.qm").arg(locale)}, QDir::Files);
        for (const auto &entry : entries) {
            auto *translator = new QTranslator(QCoreApplication::instance());
            if (translator->load(entry, dir)) {
                QCoreApplication::installTranslator(translator);
            } else {
                delete translator;
            }
        }
    }

    s_currentLanguage = locale;
}

QStringList TranslationManager::availableLanguages(const QStringList &searchPaths) {
    const QStringList paths = searchPaths.isEmpty() ? defaultSearchPaths() : searchPaths;
    QSet<QString> languages;

    for (const auto &dir : paths) {
        QDir d(dir);
        if (!d.exists())
            continue;

        const auto entries = d.entryList({QStringLiteral("*.qm")}, QDir::Files);
        for (const auto &entry : entries) {
            // filename: <target>_<locale>.qm — extract locale after last '_'
            QString baseName = entry.chopped(3); // remove ".qm"
            int underscoreIdx = baseName.indexOf(QLatin1Char('_'));
            if (underscoreIdx >= 0) {
                languages.insert(baseName.mid(underscoreIdx + 1));
            }
        }
    }

    return {languages.begin(), languages.end()};
}

QString TranslationManager::currentLanguage() {
    return s_currentLanguage;
}

} // namespace dsfw
