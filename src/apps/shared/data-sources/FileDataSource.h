#pragma once

#include <dstools/IEditorDataSource.h>

namespace dstools {

class FileDataSource : public IEditorDataSource {
    Q_OBJECT

public:
    explicit FileDataSource(QObject *parent = nullptr);
    ~FileDataSource() override = default;

    void setFile(const QString &audioFilePath, const QString &annotationPath);
    void clear();
    [[nodiscard]] bool hasFile() const;
    [[nodiscard]] const QString &annotationPath() const { return m_annotationPath; }

    [[nodiscard]] QStringList sliceIds() const override;
    [[nodiscard]] Result<DsTextDocument> loadSlice(const QString &sliceId) override;
    [[nodiscard]] Result<void> saveSlice(const QString &sliceId,
                                         const DsTextDocument &doc) override;
    [[nodiscard]] QString audioPath(const QString &sliceId) const override;

private:
    QString m_audioPath;
    QString m_annotationPath;
    QString m_formatId;
};

} // namespace dstools
