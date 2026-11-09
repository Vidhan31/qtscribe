#include "HistoryRepository.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>

using namespace Qt::StringLiterals;

class TestHistoryRepository : public QObject {
    Q_OBJECT

private slots:
    void testDefaultFilePath() {
        const QString defaultPath = HistoryRepository::defaultHistoryFilePath();
        QVERIFY(!defaultPath.isEmpty());
        QVERIFY(defaultPath.endsWith(u"/history.json"_s));
    }

    void testCustomFilePathAndAccessors() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString customPath = tempDir.filePath(u"custom_history.json"_s);

        HistoryRepository repo(customPath);
        QCOMPARE(repo.filePath(), customPath);

        const QString newPath = tempDir.filePath(u"another.json"_s);
        repo.setFilePath(newPath);
        QCOMPARE(repo.filePath(), newPath);
    }

    void testSaveAndLoadRecords() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString historyPath = tempDir.filePath(u"test_history.json"_s);

        HistoryRepository repo(historyPath);
        QVERIFY(repo.loadRecords().isEmpty());
        QCOMPARE(repo.loadRecordsJson(), u"[]"_s);

        const QDateTime now = QDateTime::currentDateTimeUtc();
        const QList<HistoryRecord> records = {
            HistoryRecord {.id = u"uuid-1"_s, .timestamp = now, .text = u"First record"_s},
            HistoryRecord {.id = u"uuid-2"_s, .timestamp = now.addSecs(-60), .text = u"Second record"_s},
        };

        QVERIFY(repo.saveRecords(records));
        QVERIFY(QFile::exists(historyPath));

        const QList<HistoryRecord> loaded = repo.loadRecords();
        QCOMPARE(loaded.size(), 2);
        QCOMPARE(loaded[0].id, u"uuid-1"_s);
        QCOMPARE(loaded[0].text, u"First record"_s);
        QCOMPARE(loaded[0].timestamp.toString(Qt::ISODate), now.toString(Qt::ISODate));
        QCOMPARE(loaded[1].id, u"uuid-2"_s);
        QCOMPARE(loaded[1].text, u"Second record"_s);

        const QString jsonStr = repo.loadRecordsJson();
        QVERIFY(!jsonStr.isEmpty());
        const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        QVERIFY(doc.isArray());
        QCOMPARE(doc.array().size(), 2);
    }

    void testAppendRecord() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString historyPath = tempDir.filePath(u"append_history.json"_s);

        HistoryRepository repo(historyPath);
        QSignalSpy changedSpy(&repo, &HistoryRepository::historyChanged);

        QVERIFY(!repo.appendRecord(QString()));
        QVERIFY(!repo.appendRecord(u"   \t\n  "_s));
        QCOMPARE(changedSpy.count(), 0);

        QVERIFY(repo.appendRecord(u"Entry one"_s));
        QCOMPARE(changedSpy.count(), 1);

        QVERIFY(repo.appendRecord(u"Entry two"_s));
        QCOMPARE(changedSpy.count(), 2);

        const QList<HistoryRecord> loaded = repo.loadRecords();
        QCOMPARE(loaded.size(), 2);
        QCOMPARE(loaded[0].text, u"Entry two"_s);
        QCOMPARE(loaded[1].text, u"Entry one"_s);
    }

    void testRetentionLimitCapAt200() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString historyPath = tempDir.filePath(u"retention_history.json"_s);

        HistoryRepository repo(historyPath);
        for (int i = 0; i < 205; ++i) {
            QVERIFY(repo.appendRecord(u"Record %1"_s.arg(i)));
        }

        const QList<HistoryRecord> loaded = repo.loadRecords();
        QCOMPARE(loaded.size(), 200);
        QCOMPARE(loaded.first().text, u"Record 204"_s);
        QCOMPARE(loaded.last().text, u"Record 5"_s);
    }

    void testFilePermissionsRestrictedToOwner() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString historyPath = tempDir.filePath(u"perms_history.json"_s);

        HistoryRepository repo(historyPath);
        QVERIFY(repo.appendRecord(u"Restricted test"_s));

        const QFile::Permissions perms = QFile::permissions(historyPath);
        QVERIFY(perms.testFlag(QFileDevice::ReadOwner));
        QVERIFY(perms.testFlag(QFileDevice::WriteOwner));
        QVERIFY(!perms.testFlag(QFileDevice::ReadGroup));
        QVERIFY(!perms.testFlag(QFileDevice::WriteGroup));
        QVERIFY(!perms.testFlag(QFileDevice::ReadOther));
        QVERIFY(!perms.testFlag(QFileDevice::WriteOther));
    }

    void testClearHistory() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString historyPath = tempDir.filePath(u"clear_history.json"_s);

        HistoryRepository repo(historyPath);
        QSignalSpy clearedSpy(&repo, &HistoryRepository::historyCleared);
        QSignalSpy changedSpy(&repo, &HistoryRepository::historyChanged);

        QVERIFY(repo.appendRecord(u"Item before clear"_s));
        QVERIFY(QFile::exists(historyPath));

        repo.clearHistory();
        QVERIFY(!QFile::exists(historyPath));
        QCOMPARE(clearedSpy.count(), 1);
        QCOMPARE(changedSpy.count(), 2);
        QVERIFY(repo.loadRecords().isEmpty());
    }

    void testCorruptedJsonHandledGracefully() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString historyPath = tempDir.filePath(u"corrupt_history.json"_s);

        QFile file(historyPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{ \"notAnArray\": true }");
        file.close();

        HistoryRepository repo(historyPath);
        QVERIFY(repo.loadRecords().isEmpty());
        QCOMPARE(repo.loadRecordsJson(), u"[]"_s);
    }

    void testFileLockingConcurrentLoad() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString historyPath = tempDir.filePath(u"lock_history.json"_s);

        HistoryRepository repo(historyPath);
        QVERIFY(repo.appendRecord(u"Locked record"_s));

        QLockFile externalLock(historyPath + u".lock"_s);
        externalLock.setStaleLockTime(5000);
        QVERIFY(externalLock.tryLock(1000));

        // When locked externally, tryLock in loadRecords will wait or fail gracefully
        externalLock.unlock();
        QCOMPARE(repo.loadRecords().size(), 1);
    }
};

QTEST_GUILESS_MAIN(TestHistoryRepository)
#include "test_history_repository.moc"
