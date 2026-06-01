#pragma once

#include "IPageDescriptor.h"

#include <QObject>

namespace dstools {

    class MinLabelPageDescriptor : public IPageDescriptor {
    public:
        QWidget *create(dsfw::AppShell *shell, IEditorDataSource *dataSource,
                        AppSettingsBackend *settingsBackend) const override;
        QString id() const override { return QStringLiteral("minlabel"); }
        QString title() const override { return QObject::tr("MinLabel"); }
    };

    class PhonemeLabelerPageDescriptor : public IPageDescriptor {
    public:
        QWidget *create(dsfw::AppShell *shell, IEditorDataSource *dataSource,
                        AppSettingsBackend *settingsBackend) const override;
        QString id() const override { return QStringLiteral("phoneme"); }
        QString title() const override { return QObject::tr("Phoneme"); }
    };

    class PitchLabelerPageDescriptor : public IPageDescriptor {
    public:
        QWidget *create(dsfw::AppShell *shell, IEditorDataSource *dataSource,
                        AppSettingsBackend *settingsBackend) const override;
        QString id() const override { return QStringLiteral("pitch"); }
        QString title() const override { return QObject::tr("Pitch"); }
    };

    class SettingsPageDescriptor : public IPageDescriptor {
    public:
        QWidget *create(dsfw::AppShell *shell, IEditorDataSource *dataSource,
                        AppSettingsBackend *settingsBackend) const override;
        QString id() const override { return QStringLiteral("settings"); }
        QString title() const override { return QObject::tr("Settings"); }
    };

    class LogPageDescriptor : public IPageDescriptor {
    public:
        QWidget *create(dsfw::AppShell *shell, IEditorDataSource *dataSource,
                        AppSettingsBackend *settingsBackend) const override;
        QString id() const override { return QStringLiteral("log"); }
        QString title() const override { return QObject::tr("Log"); }
    };

} // namespace dstools