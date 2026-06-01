#include "DictionaryPanel.h"
#include "Keys.h"

#include <dsfw/FileDialogHelper.h>
#include <dstools/PinyinG2PProvider.h>
#include <hubert-infer/DictionaryG2P.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace dstools {

DictionaryPanel::DictionaryPanel(QWidget *parent) : QWidget(parent) {}

QWidget *DictionaryPanel::createDictTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    auto *engineGroup = new QGroupBox(QStringLiteral("G2P 引擎"), w);
    auto *engineLayout = new QFormLayout(engineGroup);

    m_g2pEngineCombo = new QComboBox(engineGroup);
    m_g2pEngineCombo->addItem(QStringLiteral("内置 Pinyin (cpp-pinyin)"), QStringLiteral("pinyin"));
    m_g2pEngineCombo->addItem(QStringLiteral("词典 G2P (DictionaryG2P)"), QStringLiteral("dictionary"));
    engineLayout->addRow(QStringLiteral("G2P 引擎:"), m_g2pEngineCombo);

    layout->addWidget(engineGroup);

    auto *dictGroup = new QGroupBox(QStringLiteral("词典路径"), w);
    auto *dictLayout = new QVBoxLayout(dictGroup);

    auto *pathLayout = new QHBoxLayout;
    m_dictPath = new QLineEdit(dictGroup);
    m_dictPath->setPlaceholderText(QStringLiteral("选择词典文件 (.txt)"));
    auto *browseBtn = new QPushButton(QStringLiteral("浏览..."), dictGroup);
    pathLayout->addWidget(m_dictPath, 1);
    pathLayout->addWidget(browseBtn);
    dictLayout->addLayout(pathLayout);

    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        const QString path =
            dsfw::FileDialogHelper::getOpenFileName({this, QStringLiteral("选择词典文件"), {},
                                                     {QStringLiteral("词典文件 (*.txt)"),
                                                      QStringLiteral("所有文件 (*)")}});
        if (!path.isEmpty())
            m_dictPath->setText(path);
    });

    auto *dictNote = new QLabel(
        QStringLiteral("词典格式: 每行一条，格式为「字 音」或「词 音1 音2 ...」。\n"
                       "留空则使用内置词典。"),
        dictGroup);
    dictNote->setWordWrap(true);
    dictNote->setStyleSheet(QStringLiteral("color: gray; font-style: italic;"));
    dictLayout->addWidget(dictNote);

    layout->addWidget(dictGroup);

    auto *testGroup = new QGroupBox(QStringLiteral("G2P 测试"), w);
    auto *testLayout = new QVBoxLayout(testGroup);

    auto *inputLayout = new QHBoxLayout;
    m_g2pTestInput = new QLineEdit(testGroup);
    m_g2pTestInput->setPlaceholderText(QStringLiteral("输入文本进行 G2P 转换测试"));
    m_g2pTestBtn = new QPushButton(QStringLiteral("转换"), testGroup);
    inputLayout->addWidget(m_g2pTestInput, 1);
    inputLayout->addWidget(m_g2pTestBtn);
    testLayout->addLayout(inputLayout);

    m_g2pTestResult = new QLabel(QStringLiteral("结果将显示在这里"), testGroup);
    m_g2pTestResult->setWordWrap(true);
    m_g2pTestResult->setStyleSheet(QStringLiteral("padding: 4px; background: #2a2a2a; border-radius: 3px;"));
    testLayout->addWidget(m_g2pTestResult);

    connect(m_g2pTestBtn, &QPushButton::clicked, this, [this]() {
        QString input = m_g2pTestInput->text().trimmed();
        if (input.isEmpty()) {
            m_g2pTestResult->setText(QStringLiteral("(请输入文本)"));
            return;
        }

        QString engine = m_g2pEngineCombo->currentData().toString();
        if (engine == QStringLiteral("pinyin")) {
            dstools::PinyinG2PProvider g2p;
            auto result = g2p.convert(input.toStdString(), "zh");
            if (result) {
                QStringList phonemes;
                for (const auto &r : result.value())
                    for (const auto &ph : r.phonemes)
                        phonemes << QString::fromStdString(ph);
                m_g2pTestResult->setText(phonemes.join(QStringLiteral(" ")));
            } else {
                m_g2pTestResult->setText(QStringLiteral("错误: ") + QString::fromStdString(result.error()));
            }
        } else if (engine == QStringLiteral("dictionary")) {
            QString dictPath = m_dictPath->text().trimmed();
            if (dictPath.isEmpty()) {
                m_g2pTestResult->setText(QStringLiteral("(请先设置词典路径)"));
                return;
            }
            try {
                HFA::DictionaryG2P g2p(dictPath.toStdString(), "zh");
                auto [phSeq, wordSeq, ph2word] = g2p.convert(input.toStdString(), "zh");
                QStringList phonemes;
                for (const auto &ph : phSeq)
                    phonemes << QString::fromStdString(ph);
                m_g2pTestResult->setText(phonemes.join(QStringLiteral(" ")));
            } catch (const std::exception &e) {
                m_g2pTestResult->setText(QStringLiteral("错误: ") + QString::fromLocal8Bit(e.what()));
            }
        }
    });

    layout->addWidget(testGroup);

    connect(m_g2pEngineCombo, &QComboBox::currentTextChanged, this, [this]() {
        bool isDict = m_g2pEngineCombo->currentData().toString() == QStringLiteral("dictionary");
        m_dictPath->setEnabled(isDict);
    });
    m_dictPath->setEnabled(m_g2pEngineCombo->currentData().toString() == QStringLiteral("dictionary"));

    layout->addStretch();
    return w;
}

QWidget *DictionaryPanel::createPhNumPathCell() {
    auto *container = new QWidget;
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    auto *edit = new QLineEdit(container);
    layout->addWidget(edit, 1);
    auto *btn = new QPushButton(QStringLiteral("..."), container);
    btn->setFixedWidth(24);
    btn->setToolTip(QStringLiteral("浏览文件"));
    layout->addWidget(btn);
    connect(btn, &QPushButton::clicked, container, [edit]() {
        const QString path = dsfw::FileDialogHelper::getOpenFileName(
            {nullptr, QStringLiteral("选择文件"), {},
             {QStringLiteral("文本文件 (*.txt)"), QStringLiteral("所有文件 (*)")}});
        if (!path.isEmpty())
            edit->setText(path);
    });
    connect(edit, &QLineEdit::textChanged, container, [this]() { emit dirtyChanged(); });
    return container;
}

QLineEdit *DictionaryPanel::searchLineEditInPathCell(QWidget *cellWidget) const {
    if (!cellWidget)
        return nullptr;
    if (auto *le = qobject_cast<QLineEdit *>(cellWidget))
        return le;
    return cellWidget->findChild<QLineEdit *>();
}

QWidget *DictionaryPanel::createPhNumTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    auto *desc =
        new QLabel(QStringLiteral("按语种配置 ph_num 词典。\n"
                                  "留空使用内置默认音素分类（基于 ds-zh-pinyin-lite.txt）。\n"
                                  "词典格式: 每行「音节\\t音素1 音素2」，1个音素=元音，2个音素=辅音+元音。"),
                   w);
    desc->setWordWrap(true);
    desc->setStyleSheet(QStringLiteral("color: gray; font-style: italic; margin-bottom: 8px;"));
    layout->addWidget(desc);

    m_phNumTable = new QTableWidget(w);
    m_phNumTable->setColumnCount(4);
    m_phNumTable->setHorizontalHeaderLabels(
        {QStringLiteral("语言代码"), QStringLiteral("词典路径"), QStringLiteral("元音文件"), QStringLiteral("辅音文件")});
    m_phNumTable->horizontalHeader()->setStretchLastSection(true);
    m_phNumTable->setMinimumHeight(120);

    m_phNumTable->insertRow(0);
    m_phNumTable->setItem(0, 0, new QTableWidgetItem(QStringLiteral("zh")));
    m_phNumTable->setCellWidget(0, 1, createPhNumPathCell());
    m_phNumTable->setCellWidget(0, 2, createPhNumPathCell());
    m_phNumTable->setCellWidget(0, 3, createPhNumPathCell());

    layout->addWidget(m_phNumTable);

    auto *specialRow = new QHBoxLayout;
    auto *specialLabel = new QLabel(QStringLiteral("特殊词（逗号分隔）："), w);
    m_phNumSpecialWords = new QLineEdit(w);
    m_phNumSpecialWords->setPlaceholderText(QStringLiteral("SP, AP, EP, GS"));
    m_phNumSpecialWords->setToolTip(
        QStringLiteral("这些词在音素序列中各自对应1个音素(phNum=1)，如SP、AP等。"));
    specialRow->addWidget(specialLabel);
    specialRow->addWidget(m_phNumSpecialWords, 1);
    layout->addLayout(specialRow);

    auto *btnRow = new QHBoxLayout;
    m_phNumAddBtn = new QPushButton(QStringLiteral("添加语言"), w);
    m_phNumRemoveBtn = new QPushButton(QStringLiteral("移除所选"), w);
    btnRow->addWidget(m_phNumAddBtn);
    btnRow->addWidget(m_phNumRemoveBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    auto *note = new QLabel(
        QStringLiteral("注: 如果设置词典路径，则优先使用词典；否则依次尝试元音文件+辅音文件。\n"
                       "元音/辅音文件每行一个音素，用于指定固定的元音和辅音集合。"),
        w);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color: gray; font-style: italic; margin-top: 8px;"));
    layout->addWidget(note);

    connect(m_phNumAddBtn, &QPushButton::clicked, this, [this]() {
        int r = m_phNumTable->rowCount();
        m_phNumTable->insertRow(r);
        m_phNumTable->setItem(r, 0, new QTableWidgetItem({}));
        m_phNumTable->setCellWidget(r, 1, createPhNumPathCell());
        m_phNumTable->setCellWidget(r, 2, createPhNumPathCell());
        m_phNumTable->setCellWidget(r, 3, createPhNumPathCell());
        emit dirtyChanged();
    });
    connect(m_phNumRemoveBtn, &QPushButton::clicked, this, [this]() {
        auto sel = m_phNumTable->selectionModel()->selectedRows();
        for (int i = sel.size() - 1; i >= 0; --i)
            m_phNumTable->removeRow(sel[i].row());
        emit dirtyChanged();
    });

    layout->addStretch();
    return w;
}

QJsonObject DictionaryPanel::collectSettings() const {
    QJsonObject data;

    QJsonObject g2p;
    g2p["engine"] = m_g2pEngineCombo->currentData().toString();
    g2p["dictPath"] = m_dictPath->text();
    data["g2p"] = g2p;

    QJsonObject phNumConfig;
    phNumConfig["specialWords"] = m_phNumSpecialWords->text().trimmed();
    QJsonObject phNumLangs;
    for (int r = 0; r < m_phNumTable->rowCount(); ++r) {
        auto *langItem = m_phNumTable->item(r, 0);
        if (!langItem || langItem->text().trimmed().isEmpty())
            continue;
        QString lang = langItem->text().trimmed();
        QJsonObject langCfg;
        auto *cellWidget1 = m_phNumTable->cellWidget(r, 1);
        if (auto *le = searchLineEditInPathCell(cellWidget1))
            langCfg["dictPath"] = le->text().trimmed();
        auto *cellWidget2 = m_phNumTable->cellWidget(r, 2);
        if (auto *le = searchLineEditInPathCell(cellWidget2))
            langCfg["vowelsPath"] = le->text().trimmed();
        auto *cellWidget3 = m_phNumTable->cellWidget(r, 3);
        if (auto *le = searchLineEditInPathCell(cellWidget3))
            langCfg["consonantsPath"] = le->text().trimmed();
        phNumLangs[lang] = langCfg;
    }
    phNumConfig["languages"] = phNumLangs;
    data["phNumConfig"] = phNumConfig;

    return data;
}

void DictionaryPanel::applySettings(const QJsonObject &data) {
    const QJsonObject g2p = data["g2p"].toObject();
    {
        QString engine = g2p["engine"].toString(QStringLiteral("pinyin"));
        for (int i = 0; i < m_g2pEngineCombo->count(); ++i) {
            if (m_g2pEngineCombo->itemData(i).toString() == engine) {
                m_g2pEngineCombo->setCurrentIndex(i);
                break;
            }
        }
        m_dictPath->setText(g2p["dictPath"].toString());
        m_dictPath->setEnabled(engine == QStringLiteral("dictionary"));
    }

    const QJsonObject phNumConfig = data["phNumConfig"].toObject();
    const QString specialWords = phNumConfig["specialWords"].toString();
    if (!specialWords.isEmpty())
        m_phNumSpecialWords->setText(specialWords);
    const QJsonObject phNumLangs = phNumConfig["languages"].toObject();
    m_phNumTable->setRowCount(0);
    for (auto it = phNumLangs.begin(); it != phNumLangs.end(); ++it) {
        QString lang = it.key();
        QJsonObject langCfg = it.value().toObject();
        QString dictPath_ = langCfg["dictPath"].toString();
        QString vowelsPath = langCfg["vowelsPath"].toString();
        QString consonantsPath = langCfg["consonantsPath"].toString();
        m_phNumTable->insertRow(m_phNumTable->rowCount());
        int r = m_phNumTable->rowCount() - 1;
        auto *langItem = new QTableWidgetItem(lang);
        m_phNumTable->setItem(r, 0, langItem);
        m_phNumTable->setCellWidget(r, 1, createPhNumPathCell());
        m_phNumTable->setCellWidget(r, 2, createPhNumPathCell());
        m_phNumTable->setCellWidget(r, 3, createPhNumPathCell());
        if (auto *le = searchLineEditInPathCell(m_phNumTable->cellWidget(r, 1)))
            le->setText(dictPath_);
        if (auto *le = searchLineEditInPathCell(m_phNumTable->cellWidget(r, 2)))
            le->setText(vowelsPath);
        if (auto *le = searchLineEditInPathCell(m_phNumTable->cellWidget(r, 3)))
            le->setText(consonantsPath);
    }
    if (m_phNumTable->rowCount() == 0) {
        m_phNumTable->insertRow(0);
        m_phNumTable->setItem(0, 0, new QTableWidgetItem(QStringLiteral("zh")));
        m_phNumTable->setCellWidget(0, 1, createPhNumPathCell());
        m_phNumTable->setCellWidget(0, 2, createPhNumPathCell());
        m_phNumTable->setCellWidget(0, 3, createPhNumPathCell());
    }
}

void DictionaryPanel::connectDirtySignals() {
    connect(m_g2pEngineCombo, &QComboBox::currentTextChanged, this, &DictionaryPanel::dirtyChanged);
    connect(m_dictPath, &QLineEdit::textChanged, this, &DictionaryPanel::dirtyChanged);
    connect(m_phNumTable, &QTableWidget::cellChanged, this, &DictionaryPanel::dirtyChanged);
    connect(m_phNumSpecialWords, &QLineEdit::textEdited, this, &DictionaryPanel::dirtyChanged);
}

} // namespace dstools