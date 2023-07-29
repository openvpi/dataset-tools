#ifndef TEXTWIDGET_H
#define TEXTWIDGET_H

#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "jpg2p.h"
#include "zhg2p.h"
#include "eng2p.h"

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

    QAction *replaceAction;

    QHBoxLayout *lineLayout;
    QHBoxLayout *buttonsLayout;
    QVBoxLayout *mainLayout;

    QScopedPointer<IKg2p::ZhG2p> g2p;
    QScopedPointer<IKg2p::EnG2p> g2p_en;
    QScopedPointer<IKg2p::JpG2p> g2p_jp;
    QScopedPointer<IKg2p::ZhG2p> g2p_canton;

private:
    QString sentence() const;

    void _q_pasteButtonClicked();
    void _q_replaceButtonClicked();
    void _q_appendButtonClicked();

signals:
};

#endif // TEXTWIDGET_H
