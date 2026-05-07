#include "MinLabelEditor.h"

#include <QLineEdit>
#include <QPlainTextEdit>

namespace Minlabel {

    MinLabelEditor::MinLabelEditor(QWidget *parent) : QWidget(parent) {
        m_playWidget = new dstools::widgets::PlayWidget(this);
        m_playWidget->setObjectName("play-widget");
        m_playWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        m_textWidget = new TextWidget();
        m_textWidget->setObjectName("text-widget");

        m_fileProgress = new dstools::widgets::FileProgressTracker(
            dstools::widgets::FileProgressTracker::ProgressBarStyle, this);

        m_layout = new QVBoxLayout(this);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->addWidget(m_playWidget);
        m_layout->addWidget(m_textWidget);
        m_layout->addWidget(m_fileProgress);

        connect(m_textWidget->contentText, &QPlainTextEdit::textChanged, this, &MinLabelEditor::dataChanged);
        connect(m_textWidget->wordsText, &QLineEdit::textChanged, this, &MinLabelEditor::dataChanged);
    }

    void MinLabelEditor::loadData(const QString &labContent, const QString &rawText) {
        m_textWidget->contentText->setPlainText(labContent);
        m_textWidget->wordsText->setText(rawText);
    }

    QString MinLabelEditor::labContent() const {
        return m_textWidget->contentText->toPlainText();
    }

    QString MinLabelEditor::rawText() const {
        return m_textWidget->wordsText->text();
    }

    void MinLabelEditor::setAudioFile(const QString &path) {
        m_playWidget->openFile(path);
    }

    void MinLabelEditor::setProgress(int completed, int total) {
        m_fileProgress->setProgress(completed, total);
    }

} // namespace Minlabel
