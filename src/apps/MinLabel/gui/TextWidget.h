#ifndef TEXTWIDGET_H
#define TEXTWIDGET_H

#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <cpp-pinyin/Jyutping.h>
#include <cpp-pinyin/Pinyin.h>

class TextWidget final : public QWidget {
    Q_OBJECT
public:
    explicit TextWidget(QWidget *parent = nullptr);
    ~TextWidget() override;

    QLineEdit *wordsText;
    QPlainTextEdit *contentText;

protected:
    QPushButton *replaceButton;
    QPushButton *appendButton;
    QPushButton *pasteButton;

    QComboBox *languageCombo;

    QCheckBox *covertNum;
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
    void _q_replaceButtonClicked() const;
    void _q_appendButtonClicked() const;
    void _q_onLanguageComboIndexChanged();
};

#endif // TEXTWIDGET_H
