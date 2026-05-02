/// @file MinLabelEditor.h
/// @brief Reusable G2P label editor widget extracted from MinLabelPage.

#pragma once

#include <QVBoxLayout>
#include <QWidget>

#include <dstools/PlayWidget.h>
#include <dstools/FileProgressTracker.h>

#include "TextWidget.h"

namespace Minlabel {

    /// @brief Core label editing widget: PlayWidget + TextWidget + FileProgressTracker.
    ///
    /// Contains no file I/O, no directory browsing, no menus. Data is loaded/saved
    /// through the setter/getter API. MinLabelPage (and future DsMinLabelPage)
    /// compose this widget and connect it to their own data sources.
    class MinLabelEditor final : public QWidget {
        Q_OBJECT

    public:
        explicit MinLabelEditor(QWidget *parent = nullptr);
        ~MinLabelEditor() override = default;

        /// Populate the editors with label data.
        void loadData(const QString &labContent, const QString &rawText);

        /// @return current pronunciation/label text.
        [[nodiscard]] QString labContent() const;

        /// @return current raw text (words).
        [[nodiscard]] QString rawText() const;

        /// Open an audio file in the embedded player.
        void setAudioFile(const QString &path);

        /// Update the file-progress indicator.
        void setProgress(int completed, int total);

        /// Access the embedded play widget (e.g. for play/stop control).
        [[nodiscard]] dstools::widgets::PlayWidget *playWidget() const { return m_playWidget; }

        /// Access the embedded text widget (e.g. for external shortcut binding).
        [[nodiscard]] TextWidget *textWidget() const { return m_textWidget; }

    signals:
        /// Emitted whenever the user edits label content or raw text.
        void dataChanged();

    private:
        dstools::widgets::PlayWidget *m_playWidget = nullptr;
        TextWidget *m_textWidget = nullptr;
        dstools::widgets::FileProgressTracker *m_fileProgress = nullptr;
        QVBoxLayout *m_layout = nullptr;
    };

} // namespace Minlabel
