#pragma once

#include <dstools/IStepPlugin.h>

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

// ─── Step 1: Slice ──────────────────────────────────────────────────────

class SlicerStepPlugin : public IStepPlugin {
public:
    StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

// ─── Step 2: ASR ────────────────────────────────────────────────────────

class AsrStepPlugin : public IStepPlugin {
public:
    StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

// ─── Step 3: Label (MinLabel) ───────────────────────────────────────────

class LabelStepPlugin : public IStepPlugin {
public:
    StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

// ─── Step 4: Align (HubertFA) ───────────────────────────────────────────

class AlignStepPlugin : public IStepPlugin {
public:
    StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

// ─── Step 5: Phone (PhonemeLabeler) ─────────────────────────────────────

class PhoneStepPlugin : public IStepPlugin {
public:
    StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

// ─── Step 6: CSV ────────────────────────────────────────────────────────

class BuildCsvStepPlugin : public IStepPlugin {
public:
    StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

// ─── Step 7: MIDI (GAME) ────────────────────────────────────────────────

class GameAlignStepPlugin : public IStepPlugin {
public:
    StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

// ─── Step 8: DS ─────────────────────────────────────────────────────────

class BuildDsStepPlugin : public IStepPlugin {
public:
    StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

// ─── Step 9: Pitch (PitchLabeler) ───────────────────────────────────────

class PitchStepPlugin : public IStepPlugin {
public:
    StepPluginInfo info() const override;
    QWidget *createPage(QWidget *parent) override;
    bool isAvailable(std::string &reason) const override;
};

} // namespace dstools::labeler
