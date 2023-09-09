#include "TextWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextCodec>

TextWidget::TextWidget(QWidget *parent)
    : QWidget(parent), g2p(new IKg2p::ZhG2p("mandarin")), g2p_jp(new IKg2p::JpG2p()), g2p_en(new IKg2p::EnG2p()),
      g2p_canton(new IKg2p::ZhG2p("cantonese")), mecabYomi(mecabInit("mecabDict", "yomi")),
      mecabWakati(mecabInit("mecabDict", "wakati")) {
    wordsText = new QLineEdit();
    wordsText->setPlaceholderText("Enter mandarin here...");

    pasteButton = new QPushButton();
    pasteButton->setProperty("type", "user");
    pasteButton->setObjectName("paste-button");
    pasteButton->setIcon(QIcon(":/res/clipboard.svg"));

    lineLayout = new QHBoxLayout();
    lineLayout->setMargin(0);
    lineLayout->addWidget(wordsText);
    lineLayout->addWidget(pasteButton);

    contentText = new QPlainTextEdit();

    replaceButton = new QPushButton("Replace");
    replaceButton->setProperty("type", "user");

    replaceAction = new QAction();
    replaceAction->setShortcuts({QKeySequence(Qt::Key_Enter), QKeySequence(Qt::Key_Return)});
    replaceButton->addAction(replaceAction);

    appendButton = new QPushButton("Append");
    appendButton->setProperty("type", "user");

    languageCombo = new QComboBox();
    languageCombo->addItems({"pinyin", "romaji", "arpabet(test)", "cantonese(test)"});

    optionsLayout = new QHBoxLayout();

    manTone = new QCheckBox("Preserve tone");
    optionsLayout->addWidget(manTone);

    removeArpabetNum = new QCheckBox("Remove numbers from Arpabet");
    removeArpabetNum->hide();
    optionsLayout->addWidget(removeArpabetNum);

    removeSokuon = new QCheckBox("Remove sokuon");
    removeSokuon->hide();
    optionsLayout->addWidget(removeSokuon);

    doubleConsonant = new QCheckBox("Double consonant(\"tta\")");
    doubleConsonant->hide();
    optionsLayout->addWidget(doubleConsonant);

    canTone = new QCheckBox("Preserve tone");
    canTone->hide();
    optionsLayout->addWidget(canTone);

    buttonsLayout = new QHBoxLayout();
    buttonsLayout->setMargin(0);
    buttonsLayout->addWidget(replaceButton);
    buttonsLayout->addWidget(appendButton);
    buttonsLayout->addWidget(languageCombo);

    mainLayout = new QVBoxLayout();
    mainLayout->addLayout(lineLayout);
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addLayout(optionsLayout);
    mainLayout->addWidget(contentText);

    setLayout(mainLayout);

    connect(pasteButton, &QPushButton::clicked, this, &TextWidget::_q_pasteButtonClicked);
    connect(replaceButton, &QPushButton::clicked, this, &TextWidget::_q_replaceButtonClicked);
    connect(appendButton, &QPushButton::clicked, this, &TextWidget::_q_appendButtonClicked);

    connect(languageCombo, &QComboBox::currentTextChanged, this, &TextWidget::_q_onLanguageComboIndexChanged);

    connect(replaceAction, &QAction::triggered, this, &TextWidget::_q_replaceButtonClicked);
}

TextWidget::~TextWidget() {
}

QString TextWidget::sentence() const {
    QString words = wordsText->text();
    words.replace("\r\n", " ");
    words.replace("\n", " ");
    return words;
}

void TextWidget::_q_pasteButtonClicked() const {
    auto board = QApplication::clipboard();
    QString text = board->text();
    if (!text.isEmpty()) {
        wordsText->setText(text);
    }
}

QString filterSokuon(const QString &input) {
    QRegularExpression regex("[っッ]");
    QString result = input;
    return result.replace(regex, "");
}

void TextWidget::_q_replaceButtonClicked() {
    QString str;
    QString jpInput = removeSokuon->isChecked() ? filterSokuon(sentence()) : sentence();
    switch (languageCombo->currentIndex()) {
        case 0:
            str = g2p->convert(sentence(), manTone->isChecked());
            break;
        case 1:
            str = g2p_jp->kana2romaji(mecabConvert(jpInput), doubleConsonant->isChecked());
            break;
        case 2:
            str = g2p_en->word2arpabet(sentence(), removeArpabetNum->isChecked());
            break;
        case 3:
            str = g2p_canton->convert(sentence(), canTone->isChecked());
            break;
        default:
            break;
    }
    contentText->setPlainText(str.trimmed().simplified());
}

void TextWidget::_q_appendButtonClicked() {
    QString str;
    QString jpInput = removeSokuon->isChecked() ? filterSokuon(sentence()) : sentence();
    switch (languageCombo->currentIndex()) {
        case 0:
            str = g2p->convert(sentence(), manTone->isChecked());
            break;
        case 1:
            str = g2p_jp->kana2romaji(mecabConvert(jpInput), doubleConsonant->isChecked());
            break;
        case 2:
            str = g2p_en->word2arpabet(sentence(), removeArpabetNum->isChecked());
            break;
        case 3:
            str = g2p_canton->convert(sentence(), canTone->isChecked());
            break;
        default:
            break;
    }

    QString org = contentText->toPlainText();
    contentText->setPlainText((org.isEmpty() ? "" : org + " ") + str.trimmed().simplified());
}

void TextWidget::_q_onLanguageComboIndexChanged() {
    static QMap<QString, QList<QCheckBox *>> optionMap = {
        {"pinyin",          {manTone}                      },
        {"romaji",          {removeSokuon, doubleConsonant}},
        {"arpabet(test)",   {removeArpabetNum}             },
        {"cantonese(test)", {canTone}                      }
    };

    QString selectedLanguage = languageCombo->currentText();
    for (auto it = optionMap.begin(); it != optionMap.end(); ++it) {
        if (it.key() == selectedLanguage) {
            for (QCheckBox *control : it.value()) {
                control->show();
            }
        } else {
            for (QCheckBox *control : it.value()) {
                control->hide();
                control->setChecked(false);
            }
        }
    }
}

MeCab::Tagger *TextWidget::mecabInit(const QString &path, const QString &format) {
    QString args = "-O" + format + " -d " + path + " -r" + path + "/mecabrc";
    return MeCab::createTagger(args.toUtf8());
}

QString TextWidget::mecabConvert(const QString &input) {
    QTextCodec *codec = QTextCodec::codecForName("GBK");
    QByteArray mecabRes = mecabWakati->parse(codec->fromUnicode(input));

    QStringList out;
    foreach (auto &it, mecabRes.split(' ')) {
        QString res = codec->toUnicode(mecabYomi->parse(it));
        QStringList item = res.split("\t");
        if (item.size() > 1) {
            out.append(item[1]);
        } else if (!item.empty() && item[0] != "") {
            out.append(item[0]);
        }
    }
    return out.join(" ");
}