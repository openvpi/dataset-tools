#include "TextWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QLineEdit>
#include <QRegularExpression>
#include <QTextCodec>

#include <cpp-kana/Kana.h>

TextWidget::TextWidget(QWidget *parent)
    : QWidget(parent), g2p_man(new Pinyin::Pinyin()), g2p_canton(new Pinyin::Jyutping()),
      mecabYomi(mecabInit("mecabDict", "yomi")), mecabWakati(mecabInit("mecabDict", "wakati")) {
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
    languageCombo->addItems({"pinyin", "romaji", "cantonese"});

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

    covertNum = new QCheckBox("Convert number");
    optionsLayout->addWidget(covertNum);

    cleanRes = new QCheckBox("Clean result");
    optionsLayout->addWidget(cleanRes);

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

TextWidget::~TextWidget() = default;

QString TextWidget::sentence() const {
    QString words = wordsText->text();
    words.replace("\r\n", " ");
    words.replace("\n", " ");
    return words;
}

void TextWidget::_q_pasteButtonClicked() const {
    const auto board = QApplication::clipboard();
    const QString text = board->text();
    if (!text.isEmpty()) {
        wordsText->setText(text);
    }
}

QString filterSokuon(const QString &input) {
    static QRegularExpression regex("[っッ]");
    QString result = input;
    return result.replace(regex, "");
}

void TextWidget::_q_replaceButtonClicked() const {
    QString str;
    const QString jpInput = removeSokuon->isChecked() ? filterSokuon(sentence()) : sentence();
    switch (languageCombo->currentIndex()) {
        case 0: {
            const auto manRes = g2p_man->hanziToPinyin(
                sentence().toUtf8().toStdString(),
                manTone->isChecked() ? Pinyin::ManTone::TONE3 : Pinyin::ManTone::NORMAL,
                cleanRes->isChecked() ? Pinyin::Error::Ignore : Pinyin::Error::Default, false, true);
            str = QString::fromUtf8(manRes.toStdStr().c_str());
            break;
        }
        case 1: {
            str = QString::fromUtf8(Kana::kanaToRomaji(mecabConvert(jpInput).toUtf8().toStdString(),
                                                       Kana::Error::Default, doubleConsonant->isChecked())
                                        .toStdStr()
                                        .c_str());
            break;
        }
        case 2: {
            const auto jyutRes =
                g2p_canton->hanziToPinyin(sentence().toUtf8().toStdString(),
                                          canTone->isChecked() ? Pinyin::CanTone::TONE3 : Pinyin::CanTone::NORMAL,
                                          cleanRes->isChecked() ? Pinyin::Error::Ignore : Pinyin::Error::Default);
            str = QString::fromUtf8(jyutRes.toStdStr().c_str());
            break;
        }
        default:
            break;
    }
    contentText->setPlainText(str.trimmed().simplified());
}

void TextWidget::_q_appendButtonClicked() const {
    QString str;
    const QString jpInput = removeSokuon->isChecked() ? filterSokuon(sentence()) : sentence();
    switch (languageCombo->currentIndex()) {
        case 0: {
            const auto manRes = g2p_man->hanziToPinyin(
                sentence().toUtf8().toStdString(),
                manTone->isChecked() ? Pinyin::ManTone::TONE3 : Pinyin::ManTone::NORMAL,
                cleanRes->isChecked() ? Pinyin::Error::Ignore : Pinyin::Error::Default, false, true);
            str = QString::fromUtf8(manRes.toStdStr().c_str());
            break;
        }
        case 1: {
            str = QString::fromUtf8(Kana::kanaToRomaji(mecabConvert(jpInput).toUtf8().toStdString(),
                                                       Kana::Error::Default, doubleConsonant->isChecked())
                                        .toStdStr()
                                        .c_str());
            break;
        }
        case 2: {
            const auto jyutRes =
                g2p_canton->hanziToPinyin(sentence().toUtf8().toStdString(),
                                          canTone->isChecked() ? Pinyin::CanTone::TONE3 : Pinyin::CanTone::NORMAL,
                                          cleanRes->isChecked() ? Pinyin::Error::Ignore : Pinyin::Error::Default);
            str = QString::fromUtf8(jyutRes.toStdStr().c_str());
            break;
        }
        default:
            break;
    }

    const QString org = contentText->toPlainText();
    contentText->setPlainText((org.isEmpty() ? "" : org + " ") + str.trimmed().simplified());
}

void TextWidget::_q_onLanguageComboIndexChanged() {
    static QMap<QString, QList<QCheckBox *>> optionMap = {
        {"pinyin",    {manTone, covertNum, cleanRes} },
        {"romaji",    {removeSokuon, doubleConsonant}},
        {"cantonese", {canTone, covertNum, cleanRes} }
    };

    const QString selectedLanguage = languageCombo->currentText();
    for (auto it = optionMap.begin(); it != optionMap.end(); ++it) {
        for (QCheckBox *control : it.value()) {
            control->hide();
            control->setChecked(false);
        }
    }

    for (QCheckBox *control : optionMap[selectedLanguage]) {
        control->show();
    }
}

MeCab::Tagger *TextWidget::mecabInit(const QString &path, const QString &format) {
    const QString args = "-O" + format + " -d " + path + " -r" + path + "/mecabrc";
    return MeCab::createTagger(args.toUtf8());
}

QString TextWidget::mecabConvert(const QString &input) const {
    const QTextCodec *codec = QTextCodec::codecForName("GBK");
    const QByteArray mecabRes = mecabWakati->parse(codec->fromUnicode(input));

    QStringList out;
    foreach (auto &it, mecabRes.split(' ')) {
        QString res = codec->toUnicode(mecabYomi->parse(it));
        const QStringList item = res.split("\t");
        if (item.size() > 1) {
            out.append(item[1]);
        } else if (!item.empty() && item[0] != "") {
            out.append(item[0]);
        }
    }
    return out.join(" ");
}