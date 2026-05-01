#include <dsfw/UpdateChecker.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace dstools {

UpdateChecker::UpdateChecker(const QString &owner, const QString &repo,
                             const QString &currentVersion, QObject *parent)
    : QObject(parent), m_owner(owner), m_repo(repo), m_currentVersion(currentVersion) {
}

static bool isNewerVersion(const QString &latest, const QString &current) {
    auto parseParts = [](const QString &v) -> QList<int> {
        auto cleaned = v.startsWith('v') ? v.mid(1) : v;
        QList<int> parts;
        for (auto &s : cleaned.split('.'))
            parts.append(s.toInt());
        while (parts.size() < 3)
            parts.append(0);
        return parts;
    };
    auto l = parseParts(latest);
    auto c = parseParts(current);
    for (int i = 0; i < 3; ++i) {
        if (l[i] > c[i]) return true;
        if (l[i] < c[i]) return false;
    }
    return false;
}

void UpdateChecker::check() {
    auto *manager = new QNetworkAccessManager(this);
    QUrl url(QString("https://api.github.com/repos/%1/%2/releases/latest").arg(m_owner, m_repo));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/vnd.github.v3+json");

    auto *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        reply->deleteLater();
        manager->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit checkFailed(reply->errorString());
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit checkFailed("Invalid JSON response");
            return;
        }

        auto obj = doc.object();
        UpdateInfo info;
        info.currentVersion = m_currentVersion;
        info.latestVersion = obj["tag_name"].toString();
        info.releaseUrl = QUrl(obj["html_url"].toString());
        info.releaseNotes = obj["body"].toString();
        info.updateAvailable = isNewerVersion(info.latestVersion, info.currentVersion);

        if (info.updateAvailable)
            emit updateAvailable(info);
    });
}

} // namespace dstools
