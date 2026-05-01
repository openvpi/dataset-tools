#pragma once

/// @file UpdateChecker.h
/// @brief GitHub Releases version check (non-blocking, UI notification only).

#include <QObject>
#include <QString>
#include <QUrl>

namespace dstools {

/// @brief Result of a version check.
struct UpdateInfo {
    QString latestVersion;
    QString currentVersion;
    QUrl releaseUrl;
    QString releaseNotes;
    bool updateAvailable = false;
};

/// @brief Checks GitHub Releases API for newer versions.
class UpdateChecker : public QObject {
    Q_OBJECT
public:
    /// @param owner GitHub repository owner.
    /// @param repo GitHub repository name.
    /// @param currentVersion Current app version (e.g. "1.0.0").
    explicit UpdateChecker(const QString &owner, const QString &repo,
                           const QString &currentVersion, QObject *parent = nullptr);

    /// @brief Start an asynchronous version check.
    void check();

signals:
    /// @brief Emitted when a newer version is found.
    void updateAvailable(const UpdateInfo &info);
    /// @brief Emitted when the check fails.
    void checkFailed(const QString &error);

private:
    QString m_owner, m_repo, m_currentVersion;
};

} // namespace dstools
