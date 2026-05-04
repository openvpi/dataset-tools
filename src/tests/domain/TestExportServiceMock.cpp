#include <QtTest/QtTest>

#include <dstools/IEditorDataSource.h>
#include <ExportService.h>
#include <dstools/DsTextTypes.h>

class MockEditorDataSource : public dstools::IEditorDataSource {
    Q_OBJECT

public:
    explicit MockEditorDataSource(QObject *parent = nullptr) : IEditorDataSource(parent) {}

    QStringList m_sliceIds;
    QMap<QString, dstools::DsTextDocument> m_docs;
    QMap<QString, QString> m_audioPaths;
    QMap<QString, QStringList> m_dirtyLayers;

    [[nodiscard]] QStringList sliceIds() const override { return m_sliceIds; }

    [[nodiscard]] dstools::Result<dstools::DsTextDocument> loadSlice(const QString &sliceId) override {
        if (!m_docs.contains(sliceId))
            return dstools::Result<dstools::DsTextDocument>::Error("not found");
        return dstools::Result<dstools::DsTextDocument>::Ok(m_docs[sliceId]);
    }

    [[nodiscard]] dstools::Result<void> saveSlice(const QString &sliceId,
                                                   const dstools::DsTextDocument &doc) override {
        m_docs[sliceId] = doc;
        return dstools::Result<void>::Ok();
    }

    [[nodiscard]] QString audioPath(const QString &sliceId) const override {
        return m_audioPaths.value(sliceId);
    }

    [[nodiscard]] QStringList dirtyLayers(const QString &sliceId) const override {
        return m_dirtyLayers.value(sliceId);
    }
};

class TestExportServiceMock : public QObject {
    Q_OBJECT

private slots:
    void testValidateEmptySource() {
        dstools::ExportValidationResult result = dstools::ExportService::validate(nullptr);
        QCOMPARE(result.totalSlices, 0);
        QCOMPARE(result.readyForCsv, 0);
    }

    void testValidateWithGraphemeOnly() {
        MockEditorDataSource source;
        source.m_sliceIds = {"slice_001"};
        dstools::DsTextDocument doc;
        dstools::IntervalLayer grapheme;
        grapheme.name = QStringLiteral("grapheme");
        dstools::Boundary b;
        b.text = QStringLiteral("hello");
        grapheme.boundaries.push_back(b);
        doc.layers.push_back(grapheme);
        source.m_docs["slice_001"] = doc;

        auto result = dstools::ExportService::validate(&source);
        QCOMPARE(result.totalSlices, 1);
        QCOMPARE(result.readyForCsv, 1);
        QCOMPARE(result.missingFa, 1);
        QCOMPARE(result.missingPitch, 1);
    }

    void testValidateWithCompleteDocument() {
        MockEditorDataSource source;
        source.m_sliceIds = {"slice_001"};
        dstools::DsTextDocument doc;

        dstools::IntervalLayer grapheme;
        grapheme.name = QStringLiteral("grapheme");
        dstools::Boundary gb;
        gb.text = QStringLiteral("hello");
        grapheme.boundaries.push_back(gb);
        doc.layers.push_back(grapheme);

        dstools::IntervalLayer phoneme;
        phoneme.name = QStringLiteral("phoneme");
        dstools::Boundary pb;
        pb.text = QStringLiteral("h e l o");
        phoneme.boundaries.push_back(pb);
        doc.layers.push_back(phoneme);

        dstools::IntervalLayer phNum;
        phNum.name = QStringLiteral("ph_num");
        dstools::Boundary nb;
        nb.text = QStringLiteral("1 1 1 1");
        phNum.boundaries.push_back(nb);
        doc.layers.push_back(phNum);

        dstools::CurveLayer pitch;
        pitch.name = QStringLiteral("pitch");
        pitch.values = {440000, 450000};
        doc.curves.push_back(pitch);

        source.m_docs["slice_001"] = doc;

        auto result = dstools::ExportService::validate(&source);
        QCOMPARE(result.totalSlices, 1);
        QCOMPARE(result.readyForCsv, 1);
        QCOMPARE(result.missingFa, 0);
        QCOMPARE(result.missingPhNum, 0);
        QCOMPARE(result.missingPitch, 0);
    }

    void testAutoCompleteNullSource() {
        dstools::ExportService::autoCompleteSlice(nullptr, "slice_001", nullptr, nullptr, nullptr, nullptr);
    }

    void testAutoCompleteMissingSlice() {
        MockEditorDataSource source;
        source.m_sliceIds = {"slice_001"};
        dstools::ExportService::autoCompleteSlice(&source, "slice_999", nullptr, nullptr, nullptr, nullptr);
    }
};

QTEST_MAIN(TestExportServiceMock)
#include "TestExportServiceMock.moc"
