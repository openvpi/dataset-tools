#pragma once

#include <ISettingsBackend.h>
#include <dstools/DsProject.h>

namespace dstools {

class ProjectSettingsBackend : public ISettingsBackend {
    Q_OBJECT
public:
    explicit ProjectSettingsBackend(QObject *parent = nullptr);

    void setProject(DsProject *project);
    DsProject *project() const;

    QJsonObject load() const override;
    void save(const QJsonObject &data) override;

private:
    DsProject *m_project = nullptr;
};

} // namespace dstools
