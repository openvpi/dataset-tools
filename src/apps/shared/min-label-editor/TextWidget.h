/// @file TextWidget.h
/// @brief G2P text conversion widget for MinLabel.

#pragma once
#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <cpp-pinyin/Jyutping.h>
#include <cpp-pinyin/Pinyin.h>

namespace Minlabel {
    /// @brief Widget with text input, language selection (Mandarin/Cantonese/Japanese), and Pinyin/Jyutping conversion with configurable tone and formatting options.
    class TextWidget final : public QWidget {
        Q_OBJECT
    public:
        /// @brief Constructs the text widget.
        /// @param parent Optional parent widget.
        explicit TextWidget(QWidget *parent = nullptr);
        ~TextWidget() override;

        QLineEdit *wordsText;         ///< Single-line input for words.
        QPlainTextEdit *contentText;  ///< Multi-line pronunciation output.

        void textToPronunciation(bool append = false) const;

    protected:
        QPushButton *replaceButton;
        QPushButton *appendButton;
        QPushButton *pasteButton;

        QComboBox *languageCombo;

        QCheckBox *convertNum;
        QCheckBox *cleanRes;

        QCheckBox *manTone;
        QCheckBox *canTone;
        QCheckBox *removeArpabetNum;
        QCheckBox *removeSokuon;
        QCheckBox *doubleConsonant;

        QAction *replaceAction;

        QHBoxLayout *lineLayout;
        QHBoxLayout *buttonsLayout;
        QHBoxLayout *optionsLayout;
        QVBoxLayout *mainLayout;

        QScopedPointer<Pinyin::Pinyin> g2p_man;
        QScopedPointer<Pinyin::Jyutping> g2p_canton;

    private:
        QString sentence() const;

        void _q_pasteButtonClicked() const;
        void _q_onLanguageComboIndexChanged();
    };
}
