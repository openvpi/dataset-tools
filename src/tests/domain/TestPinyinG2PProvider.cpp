#include <QTest>

#include <dstools/PinyinG2PProvider.h>
#include <dsfw/IG2PProvider.h>

using namespace dstools;

class TestPinyinG2PProvider : public QObject {
    Q_OBJECT
private slots:
    void testProviderName() {
        PinyinG2PProvider provider;
        QCOMPARE(QString::fromLatin1(provider.providerName()), QStringLiteral("PinyinG2P"));
    }

    void testConvert_unsupportedLanguage() {
        PinyinG2PProvider provider;
        auto result = provider.convert("hello", "en");
        QVERIFY(!result.ok());
        QVERIFY(result.error().find("zh") != std::string::npos);
    }

    void testConvert_emptyText() {
        PinyinG2PProvider provider;
        auto result = provider.convert("", "zh");
        QVERIFY(result.ok());
        QVERIFY(result.value().empty());
    }

    void testConvert_whitespaceOnly() {
        PinyinG2PProvider provider;
        auto result = provider.convert("  \n\t  ", "zh");
        QVERIFY(result.ok());
        QVERIFY(result.value().empty());
    }

    void testConvert_singleWord() {
        PinyinG2PProvider provider;
        auto result = provider.convert("ni", "zh");
        QVERIFY(result.ok());
        auto items = result.value();
        QCOMPARE(items.size(), size_t(1));
        QCOMPARE(QString::fromStdString(items[0].word), QStringLiteral("ni"));
    }

    void testConvert_multipleWords() {
        PinyinG2PProvider provider;
        auto result = provider.convert("ni hao", "zh");
        QVERIFY(result.ok());
        auto items = result.value();
        QCOMPARE(items.size(), size_t(2));
        QCOMPARE(QString::fromStdString(items[0].word), QStringLiteral("ni"));
        QCOMPARE(QString::fromStdString(items[1].word), QStringLiteral("hao"));
    }

    void testConvert_multipleWordsWithTabs() {
        PinyinG2PProvider provider;
        auto result = provider.convert("ni\thao", "zh");
        QVERIFY(result.ok());
        auto items = result.value();
        QCOMPARE(items.size(), size_t(2));
    }

    void testConvert_multipleWordsWithNewlines() {
        PinyinG2PProvider provider;
        auto result = provider.convert("ni\nhao", "zh");
        QVERIFY(result.ok());
        auto items = result.value();
        QCOMPARE(items.size(), size_t(2));
    }

    void testConvert_zhCN() {
        PinyinG2PProvider provider;
        auto result = provider.convert("test", "zh-CN");
        QVERIFY(result.ok());
        auto items = result.value();
        QCOMPARE(items.size(), size_t(1));
    }

    void testConvertWord_unsupportedLanguage() {
        PinyinG2PProvider provider;
        auto result = provider.convertWord("hello", "ja");
        QVERIFY(!result.ok());
        QVERIFY(result.error().find("zh") != std::string::npos);
    }

    void testConvertWord_valid() {
        PinyinG2PProvider provider;
        auto result = provider.convertWord("ni", "zh");
        QVERIFY(result.ok());
        QCOMPARE(QString::fromStdString(result.value().word), QStringLiteral("ni"));
    }

    void testConvertWord_empty() {
        PinyinG2PProvider provider;
        auto result = provider.convertWord("", "zh");
        QVERIFY(result.ok());
        QCOMPARE(QString::fromStdString(result.value().word), QStringLiteral(""));
        QVERIFY(result.value().phonemes.empty());
    }
};

QTEST_GUILESS_MAIN(TestPinyinG2PProvider)
#include "TestPinyinG2PProvider.moc"