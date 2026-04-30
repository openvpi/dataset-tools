#pragma once

#include <dsfw/IStepPlugin.h>

class SlicerPage;
class LyricFAPage;
class HubertFAPage;

namespace Minlabel {
class MinLabelPage;
}
namespace dstools::phonemelabeler {
class PhonemeLabelerPage;
}
namespace dstools::pitchlabeler {
class PitchLabelerPage;
}

namespace dstools::labeler {

class BuildCsvPage;
class BuildDsPage;
class GameAlignPage;

class SlicerStepPlugin : public dsfw::IStepPlugin {
public:
    dsfw::StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

class AsrStepPlugin : public dsfw::IStepPlugin {
public:
    dsfw::StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

class LabelStepPlugin : public dsfw::IStepPlugin {
public:
    dsfw::StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

class AlignStepPlugin : public dsfw::IStepPlugin {
public:
    dsfw::StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

class PhoneStepPlugin : public dsfw::IStepPlugin {
public:
    dsfw::StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

class BuildCsvStepPlugin : public dsfw::IStepPlugin {
public:
    dsfw::StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

class GameAlignStepPlugin : public dsfw::IStepPlugin {
public:
    dsfw::StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

class BuildDsStepPlugin : public dsfw::IStepPlugin {
public:
    dsfw::StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

class PitchStepPlugin : public dsfw::IStepPlugin {
public:
    dsfw::StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

} // namespace dstools::labeler
