#pragma once

#include <QFileDialog>
#include <QString>
#include <QStringList>
#include <QWidget>

namespace dsfw {

class FileDialogHelper {
public:
    struct Options {
        QWidget *parent = nullptr;
        QString title;
        QString defaultDir;
        QStringList nameFilters;
        QString defaultSuffix;
        bool resolveSymlinks = true;
    };

    static QString getOpenFileName(const Options &opts) {
        return QFileDialog::getOpenFileName(opts.parent, opts.title, opts.defaultDir,
                                            opts.nameFilters.join(QStringLiteral(";;")),
                                            nullptr, QFileDialog::Options());
    }

    static QStringList getOpenFileNames(const Options &opts) {
        return QFileDialog::getOpenFileNames(opts.parent, opts.title, opts.defaultDir,
                                             opts.nameFilters.join(QStringLiteral(";;")),
                                             nullptr, QFileDialog::Options());
    }

    static QString getExistingDirectory(const Options &opts) {
        return QFileDialog::getExistingDirectory(opts.parent, opts.title, opts.defaultDir,
                                                 QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    }

    static QString getSaveFileName(const Options &opts) {
        QString suffix = opts.defaultSuffix;
        return QFileDialog::getSaveFileName(opts.parent, opts.title, opts.defaultDir,
                                            opts.nameFilters.join(QStringLiteral(";;")),
                                            opts.resolveSymlinks ? nullptr : &suffix,
                                            QFileDialog::Options());
    }
};

} // namespace dsfw