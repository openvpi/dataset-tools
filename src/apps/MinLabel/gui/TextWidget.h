#ifndef TEXTWIDGET_H
#define TEXTWIDGET_H

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "cantonese.h"
#include "eng2p.h"
#include "jpg2p.h"
#include "mandarin.h"

#include "mecab/mecab.h"
class TextWidget : public QWidget {
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

    QScopedPointer<IKg2p::Mandarin> g2p_man;
    QScopedPointer<IKg2p::Cantonese> g2p_canton;
    QScopedPointer<IKg2p::EnG2p> g2p_en;
    QScopedPointer<IKg2p::JpG2p> g2p_jp;

private:
    MeCab::Tagger *mecabYomi;
    MeCab::Tagger *mecabWakati;
    QString sentence() const;
    static MeCab::Tagger *mecabInit(const QString &path = "mecabDict", const QString &format = "wakati");
    QString mecabConvert(const QString &input);

    void _q_pasteButtonClicked() const;
    void _q_replaceButtonClicked();
    void _q_appendButtonClicked();
    void _q_onLanguageComboIndexChanged();
};

#endif // TEXTWIDGET_H
