#pragma once

#include <QWidget>
#include <QString>
#include <QIcon>

#include <string>

namespace dstools {

struct StepPluginInfo {
    QString id;
    QString displayName;
    QString description;
    QIcon icon;
    int suggestedPosition = -1;
    bool isBatchStep = true;
};

class IStepPlugin {
public:
    virtual ~IStepPlugin() = default;
    virtual StepPluginInfo info() const = 0;
    virtual QWidget *createPage(QWidget *parent) = 0;
    virtual bool isAvailable(std::string &reason) const = 0;
};

class StubStepPlugin : public IStepPlugin {
public:
    StepPluginInfo info() const override {
        return {"stub", "Stub Plugin", "Not implemented", {}, -1, true};
    }
    QWidget *createPage(QWidget *parent) override {
        Q_UNUSED(parent);
        return nullptr;
    }
    bool isAvailable(std::string &reason) const override {
        reason = "Step plugin not implemented";
        return false;
    }
};

} // namespace dstools
