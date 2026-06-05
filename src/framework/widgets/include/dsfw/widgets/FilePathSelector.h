#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <dsfw/widgets/WidgetsGlobal.h>

namespace dsfw::widgets {

class DSFW_WIDGETS_API FilePathSelector : public QObject {
    Q_OBJECT
public:
    enum class Mode { OpenFile, OpenFiles, SaveFile, OpenDirectory };

    struct Config {
        Mode mode = Mode::OpenFile;
        QString title;
        QStringList nameFilters;
        QString defaultSuffix;
        QString settingsKey;
        QString initialPath;
    };

    explicit FilePathSelector(const Config &config, QWidget *parent = nullptr);

    QString selectedPath() const;
    QStringList selectedPaths() const;

    QString exec();

signals:
    void pathSelected(const QString &path);
    void pathsSelected(const QStringList &paths);

private:
    Config m_config;
    QString m_selectedPath;
    QStringList m_selectedPaths;
};

} // namespace dsfw::widgets