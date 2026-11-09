#include "TranscriptionModel.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>

using namespace Qt::StringLiterals;

class TestTranscriptionModel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { qputenv("QTSCRIBE_DISABLE_REMOTE_ATTACH", "1"); }

    void testSourceModelRejectsEmptyAndWhitespace() {
        TranscriptionSourceModel source;
        QSignalSpy countSpy(&source, &TranscriptionSourceModel::countChanged);

        source.addRecord(QString());
        source.addRecord(u""_s);
        source.addRecord(u"   \t\n  "_s);

        QCOMPARE(source.count(), 0);
        QCOMPARE(source.rowCount(), 0);
        QCOMPARE(countSpy.count(), 0);
    }

    void testSourceModelInsertionOrderAndRoles() {
        TranscriptionSourceModel source;
        source.addRecord(u"First transcription"_s);
        source.addRecord(u"Second transcription"_s);

        QCOMPARE(source.count(), 2);
        QCOMPARE(source.rowCount(), 2);

        // Prepends, so newest record is at row 0
        const QModelIndex firstRow = source.index(0, 0);
        QVERIFY(firstRow.isValid());
        QCOMPARE(source.data(firstRow, static_cast<int>(TranscriptionSourceModel::Roles::TextRole)).toString(),
                 u"Second transcription"_s);
        QVERIFY(!source.data(firstRow, static_cast<int>(TranscriptionSourceModel::Roles::IdRole)).toString().isEmpty());
        QVERIFY(source.data(firstRow, static_cast<int>(TranscriptionSourceModel::Roles::TimestampRole)).isValid());
        QVERIFY(!source.data(firstRow, static_cast<int>(TranscriptionSourceModel::Roles::FormattedTimestampRole))
                     .toString()
                     .isEmpty());

        const QModelIndex secondRow = source.index(1, 0);
        QCOMPARE(source.data(secondRow, static_cast<int>(TranscriptionSourceModel::Roles::TextRole)).toString(),
                 u"First transcription"_s);

        const auto roles = source.roleNames();
        QCOMPARE(roles.value(static_cast<int>(TranscriptionSourceModel::Roles::IdRole)), "id");
        QCOMPARE(roles.value(static_cast<int>(TranscriptionSourceModel::Roles::TextRole)), "text");
    }

    void testSourceModelCapAt200() {
        TranscriptionSourceModel source;

        // Add 205 records
        for (int i = 0; i < 205; ++i) {
            source.addRecord(u"Record %1"_s.arg(i));
        }

        // Must cap at 200
        QCOMPARE(source.count(), 200);
        QCOMPARE(source.rowCount(), 200);

        // Row 0 is the newest record (204)
        QCOMPARE(
            source.data(source.index(0, 0), static_cast<int>(TranscriptionSourceModel::Roles::TextRole)).toString(),
            u"Record 204"_s);

        // Row 199 is the oldest preserved record (5) — records 0, 1, 2, 3, 4 were evicted
        QCOMPARE(
            source.data(source.index(199, 0), static_cast<int>(TranscriptionSourceModel::Roles::TextRole)).toString(),
            u"Record 5"_s);
    }

    void testSourceModelClearAll() {
        TranscriptionSourceModel source;
        source.addRecord(u"Item 1"_s);
        source.addRecord(u"Item 2"_s);
        QCOMPARE(source.count(), 2);

        QSignalSpy countSpy(&source, &TranscriptionSourceModel::countChanged);
        source.clearAll();

        QCOMPARE(source.count(), 0);
        QCOMPARE(countSpy.count(), 1);

        // Calling clearAll on empty is a safe no-op
        source.clearAll();
        QCOMPARE(countSpy.count(), 1);
    }

    void testProxyFilterBehavior() {
        TranscriptionModel model;
        model.addRecord(u"Bug report: crash on startup"_s);
        model.addRecord(u"Feature request: add dark mode"_s);
        model.addRecord(u"Bug report: memory leak in audio stream"_s);
        model.addRecord(u"General notes from meeting"_s);

        QCOMPARE(model.totalCount(), 4);
        QCOMPARE(model.count(), 4);

        QSignalSpy filterSpy(&model, &TranscriptionModel::searchFilterChanged);
        QSignalSpy countSpy(&model, &TranscriptionModel::countChanged);

        // Case-insensitive filtering
        model.setSearchFilter(u"bug report"_s);
        QCOMPARE(filterSpy.count(), 1);
        QCOMPARE(model.searchFilter(), u"bug report"_s);
        QCOMPARE(model.totalCount(), 4);
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.filteredCount(), 2);

        // Verify the filtered items
        QCOMPARE(model.data(model.index(0, 0), static_cast<int>(TranscriptionModel::Roles::TextRole)).toString(),
                 u"Bug report: memory leak in audio stream"_s);
        QCOMPARE(model.data(model.index(1, 0), static_cast<int>(TranscriptionModel::Roles::TextRole)).toString(),
                 u"Bug report: crash on startup"_s);

        // Search for non-existent text
        model.setSearchFilter(u"nonexistent_token_xyz"_s);
        QCOMPARE(model.count(), 0);
        QCOMPARE(model.totalCount(), 4);

        // Reset search filter restores full list
        model.setSearchFilter(QString());
        QCOMPARE(model.count(), 4);
        QCOMPARE(model.totalCount(), 4);
    }

    void testLoadHistoryRestoresOriginalFields() {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());

        const QString expectedId = u"test-uuid-1234"_s;
        const QString dateStr = u"2026-01-15T12:30:00Z"_s;
        const QDateTime expectedTimestamp = QDateTime::fromString(dateStr, Qt::ISODate);
        const QString expectedText = u"Archived transcription record"_s;

        QJsonArray records;
        QJsonObject obj;
        obj[u"id"_s] = expectedId;
        obj[u"timestamp"_s] = dateStr;
        obj[u"text"_s] = expectedText;
        records.append(obj);

        tempFile.write(QJsonDocument(records).toJson());
        tempFile.close();

        TranscriptionModel model;
        model.loadHistory(tempFile.fileName());

        QCOMPARE(model.count(), 1);
        QCOMPARE(model.data(model.index(0, 0), static_cast<int>(TranscriptionModel::Roles::IdRole)).toString(),
                 expectedId);
        QCOMPARE(model.data(model.index(0, 0), static_cast<int>(TranscriptionModel::Roles::TimestampRole)).toDateTime(),
                 expectedTimestamp);
        QCOMPARE(model.data(model.index(0, 0), static_cast<int>(TranscriptionModel::Roles::TextRole)).toString(),
                 expectedText);
    }

    void testLoadFromJson() {
        const QString json =
            u"[{\"id\":\"custom-id\",\"timestamp\":\"2026-02-01T10:00:00Z\",\"text\":\"Loaded from JSON\"}]"_s;
        TranscriptionModel model;
        model.loadFromJson(json);

        QCOMPARE(model.count(), 1);
        QCOMPARE(model.data(model.index(0, 0), static_cast<int>(TranscriptionModel::Roles::IdRole)).toString(),
                 u"custom-id"_s);
        QCOMPARE(model.data(model.index(0, 0), static_cast<int>(TranscriptionModel::Roles::TextRole)).toString(),
                 u"Loaded from JSON"_s);
    }

    void testClearAllEmptiesModel() {
        TranscriptionModel model;
        model.addRecord(u"To be cleared 1"_s);
        model.addRecord(u"To be cleared 2"_s);
        QCOMPARE(model.count(), 2);

        model.clearAll();
        QCOMPARE(model.count(), 0);
    }
};

QTEST_GUILESS_MAIN(TestTranscriptionModel)
#include "test_transcription_model.moc"
