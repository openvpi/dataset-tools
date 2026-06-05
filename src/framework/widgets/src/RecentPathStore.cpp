#include <dsfw/widgets/RecentPathStore.h>
#include <dsfw/AppSettings.h>

#include <QFileInfo>

namespace dsfw::widgets {

QString RecentPathStore::load(const QString &settingsKey) {
    if (settingsKey.isEmpty())
        return {};
    static dsfw::AppSettings s_settings(QStringLiteral("PathSelector"));
    return s_settings.getRawString(settingsKey.toUtf8().constData(), {});
}

void RecentPathStore::save(const QString &settingsKey, const QString &path) {
    if (settingsKey.isEmpty())
        return;
    static dsfw::AppSettings s_settings(QStringLiteral("PathSelector"));
    const QFileInfo info(path);
    const QString dir = info.isDir() ? path : info.absolutePath();
    s_settings.setRawString(settingsKey.toUtf8().constData(), dir);
}

} // namespace dsfw::widgets
