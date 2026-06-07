#pragma once

#include <QMap>
#include <QString>
#include <dstools/IEditorDataSource.h>

namespace dstools {

    class DirectoryDataSource : public IEditorDataSource {
        Q_OBJECT

    public:
        explicit DirectoryDataSource(QObject *parent = nullptr);

        void setWorkingDirectory(const QString &dir);
        QString workingDirectory() const;

        void refresh();

        int getSliceCount() const override;
        QStringList sliceIds() const override;
        [[nodiscard]] dsfw::Result<DsTextDocument> loadSlice(const QString &sliceId) override;
        [[nodiscard]] dsfw::Result<void> saveSlice(const QString &sliceId, const DsTextDocument &doc) override;
        QString audioPath(const QString &sliceId) const override;

    private:
        QString dstextPath(const QString &sliceId) const;
        QString sliceAudioPath(const QString &sliceId) const;

        QString m_workingDir;
        QStringList m_sliceIds;
    };

} // namespace dstools
