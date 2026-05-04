#pragma once

#include <dstools/IEditorDataSource.h>

#include <QMap>
#include <QString>

namespace dstools {

class DirectoryDataSource : public IEditorDataSource {
    Q_OBJECT

public:
    explicit DirectoryDataSource(QObject *parent = nullptr);

    void setWorkingDirectory(const QString &dir);
    QString workingDirectory() const;

    void refresh();

    QStringList sliceIds() const override;
    Result<DsTextDocument> loadSlice(const QString &sliceId) override;
    Result<void> saveSlice(const QString &sliceId, const DsTextDocument &doc) override;
    QString audioPath(const QString &sliceId) const override;

private:
    QString dstextPath(const QString &sliceId) const;
    QString sliceAudioPath(const QString &sliceId) const;

    QString m_workingDir;
    QStringList m_sliceIds;
};

} // namespace dstools
