#include "StepPlugins.h"

#include <QApplication>
#include <QDir>

// Page includes
#include "SlicerPage.h"
#include "LyricFAPage.h"
#include "HubertFAPage.h"
#include "MinLabelPage.h"
#include "PhonemeLabelerPage.h"
#include "PitchLabelerPage.h"
#include "pages/BuildCsvPage.h"
#include "pages/BuildDsPage.h"
#include "pages/GameAlignPage.h"

namespace dstools::labeler {

// ─── Helpers ────────────────────────────────────────────────────────────

static QString modelDir() {
#ifdef Q_OS_MAC
    return QDir::cleanPath(QApplication::applicationDirPath() + "/../Resources/model");
#else
    return QDir::cleanPath(QApplication::applicationDirPath() + QDir::separator() + "model");
#endif
}

static bool modelDirExists(const QString &subdir, std::string &reason) {
    const QDir dir(modelDir() + QDir::separator() + subdir);
    if (!dir.exists()) {
        reason = "Model directory not found: " + dir.absolutePath().toStdString();
        return false;
    }
    return true;
}

// ─── Step 1: Slice ──────────────────────────────────────────────────────

dsfw::StepPluginInfo SlicerStepPlugin::info() const {
    return {"slicer", "Audio Slicer", "RMS-based audio slicing", {}, 0, true};
}

QWidget *SlicerStepPlugin::createPage(QWidget *parent) {
    return new SlicerPage(parent);
}

bool SlicerStepPlugin::isAvailable(std::string & /*reason*/) const {
    return true; // No external dependencies
}

// ─── Step 2: ASR ────────────────────────────────────────────────────────

dsfw::StepPluginInfo AsrStepPlugin::info() const {
    return {"asr", "ASR (LyricFA)", "FunASR lyric forced alignment", {}, 1, true};
}

QWidget *AsrStepPlugin::createPage(QWidget *parent) {
    return new LyricFAPage(parent);
}

bool AsrStepPlugin::isAvailable(std::string &reason) const {
    return modelDirExists("ParaformerModel", reason);
}

// ─── Step 3: Label (MinLabel) ───────────────────────────────────────────

dsfw::StepPluginInfo LabelStepPlugin::info() const {
    return {"label", "Label (MinLabel)", "Audio labeling with G2P conversion", {}, 2, false};
}

QWidget *LabelStepPlugin::createPage(QWidget *parent) {
    return new Minlabel::MinLabelPage(parent);
}

bool LabelStepPlugin::isAvailable(std::string & /*reason*/) const {
    return true; // No external model required
}

// ─── Step 4: Align (HubertFA) ───────────────────────────────────────────

dsfw::StepPluginInfo AlignStepPlugin::info() const {
    return {"align", "Align (HubertFA)", "HuBERT phoneme forced alignment", {}, 3, true};
}

QWidget *AlignStepPlugin::createPage(QWidget *parent) {
    return new HubertFAPage(parent);
}

bool AlignStepPlugin::isAvailable(std::string &reason) const {
    return modelDirExists("HfaModel", reason);
}

// ─── Step 5: Phone (PhonemeLabeler) ─────────────────────────────────────

dsfw::StepPluginInfo PhoneStepPlugin::info() const {
    return {"phone", "Phone (PhonemeLabeler)", "TextGrid phoneme boundary editing", {}, 4, false};
}

QWidget *PhoneStepPlugin::createPage(QWidget *parent) {
    return new dstools::phonemelabeler::PhonemeLabelerPage(parent);
}

bool PhoneStepPlugin::isAvailable(std::string & /*reason*/) const {
    return true; // No external model required
}

// ─── Step 6: CSV ────────────────────────────────────────────────────────

dsfw::StepPluginInfo BuildCsvStepPlugin::info() const {
    return {"csv", "Build CSV", "Generate transcription CSV from labels", {}, 5, true};
}

QWidget *BuildCsvStepPlugin::createPage(QWidget *parent) {
    return new BuildCsvPage(parent);
}

bool BuildCsvStepPlugin::isAvailable(std::string & /*reason*/) const {
    return true;
}

// ─── Step 7: MIDI (GAME) ────────────────────────────────────────────────

dsfw::StepPluginInfo GameAlignStepPlugin::info() const {
    return {"midi", "MIDI (GAME)", "GAME audio-to-MIDI transcription", {}, 6, true};
}

QWidget *GameAlignStepPlugin::createPage(QWidget *parent) {
    return new GameAlignPage(parent);
}

bool GameAlignStepPlugin::isAvailable(std::string &reason) const {
    // GAME model directory contains config.json + ONNX files
    const QDir dir(modelDir());
    const QFileInfo configFile(dir.filePath("config.json"));
    if (!configFile.exists()) {
        reason = "GAME model not found: expected config.json in " + dir.absolutePath().toStdString();
        return false;
    }
    return true;
}

// ─── Step 8: DS ─────────────────────────────────────────────────────────

dsfw::StepPluginInfo BuildDsStepPlugin::info() const {
    return {"ds", "Build DS", "Build DiffSinger .ds files with F0 extraction", {}, 7, true};
}

QWidget *BuildDsStepPlugin::createPage(QWidget *parent) {
    return new BuildDsPage(parent);
}

bool BuildDsStepPlugin::isAvailable(std::string &reason) const {
    // RMVPE model is user-selected via path selector, not a fixed location.
    // Always available; model load errors are handled at runtime.
    Q_UNUSED(reason)
    return true;
}

// ─── Step 9: Pitch (PitchLabeler) ───────────────────────────────────────

dsfw::StepPluginInfo PitchStepPlugin::info() const {
    return {"pitch", "Pitch (PitchLabeler)", "DiffSinger .ds F0 curve editing", {}, 8, false};
}

QWidget *PitchStepPlugin::createPage(QWidget *parent) {
    return new dstools::pitchlabeler::PitchLabelerPage(parent);
}

bool PitchStepPlugin::isAvailable(std::string & /*reason*/) const {
    return true; // No external model required
}

} // namespace dstools::labeler
