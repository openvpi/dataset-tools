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

#include "eng2p.h"
#include "jpg2p.h"
#include "zhg2p.h"

#include "mecab/mecab.h"
class TextWidget : public QWidget {
    Q_OBJECT
public:
    explicit TextWidget(QWidget *parent = nullptr);
    ~TextWidget();

    QLineEdit *wordsText;
    QPlainTextEdit *contentText;

protected:
    QPushButton *replaceButton;
    QPushButton *appendButton;
    QPushButton *pasteButton;

    QComboBox *languageCombo;

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

    QScopedPointer<IKg2p::ZhG2p> g2p;
    QScopedPointer<IKg2p::EnG2p> g2p_en;
    QScopedPointer<IKg2p::JpG2p> g2p_jp;
    QScopedPointer<IKg2p::ZhG2p> g2p_canton;

private:
    mecab_t *mecab;
    QString sentence() const;
    static mecab_t *mecabInit(const QString &path = "mecabDict");
    QString mecabConvert(const QString &input);

    void _q_pasteButtonClicked();
    void _q_replaceButtonClicked();
    void _q_appendButtonClicked();
    void _q_onLanguageComboIndexChanged();
};

#endif // TEXTWIDGET_H
