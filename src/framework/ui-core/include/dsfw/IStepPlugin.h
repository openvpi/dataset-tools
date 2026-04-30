#pragma once

/// @file IStepPlugin.h
/// @brief Step plugin interface for extending the dataset labeling wizard with custom steps.

#include <QWidget>
#include <QString>
#include <QIcon>

#include <string>

namespace dsfw {

/// @brief Descriptor for a step plugin.
struct StepPluginInfo {
    QString id;                ///< Unique plugin identifier.
    QString displayName;       ///< Human-readable name shown in the wizard.
    QString description;       ///< Short description of what the step does.
    QIcon icon;                ///< Icon for the step in the sidebar.
    int suggestedPosition = -1; ///< Suggested insertion index (-1 = append).
    bool isBatchStep = true;   ///< True if this is a batch processing step.
};

/// @brief Abstract interface for a wizard step plugin.
class IStepPlugin {
public:
    virtual ~IStepPlugin() = default;
    /// @brief Return metadata about this plugin.
    /// @return StepPluginInfo descriptor.
    virtual StepPluginInfo info() const = 0;
    /// @brief Create the page widget for this step.
    /// @param parent Parent widget.
    /// @return New widget (ownership transferred to caller).
    virtual QWidget *createPage(QWidget *parent) = 0;
    /// @brief Check whether this plugin is available (e.g. model files present).
    /// @param reason Populated with unavailability reason if returning false.
    /// @return True if the plugin can be used.
    virtual bool isAvailable(std::string &reason) const = 0;
};

/// @brief No-op step plugin stub for use as a placeholder.
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

} // namespace dsfw
