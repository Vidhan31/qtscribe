#include "ClipboardManager.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

class TestableClipboardManager : public ClipboardManager {
    Q_OBJECT
public:
    explicit TestableClipboardManager(QObject* parent = nullptr) : ClipboardManager(parent) {}

    bool mockToolsExist = false;
    QString customToolPath;

    QString wlToolPath(const QString& toolName) const override {
        Q_UNUSED(toolName);
        if (mockToolsExist) {
            return customToolPath.isEmpty() ? u"/mock/bin/wl-tool"_s : customToolPath;
        }
        return QString();
    }
};

class TestClipboardManager : public QObject {
    Q_OBJECT

private slots:
    void testMissingToolsReportCleanErrors() {
        TestableClipboardManager manager;
        manager.mockToolsExist = false;

        QSignalSpy errorSpy(&manager, &ClipboardManager::lastErrorChanged);

        const QString backup = manager.backupText();
        QVERIFY(backup.isEmpty());
        QVERIFY(manager.lastError().contains(u"wl-paste not found in PATH"_s));
        QCOMPARE(errorSpy.count(), 1);

        const bool setOk = manager.setText(u"sample text"_s);
        QVERIFY(!setOk);
        QVERIFY(manager.lastError().contains(u"wl-copy not found in PATH"_s));
        QCOMPARE(errorSpy.count(), 2);
    }

    void testScheduleAndCancelPendingRestore() {
        TestableClipboardManager manager;
        QVERIFY(!manager.hasActiveBackup());

        manager.scheduleRestore(u"Previous text"_s, true, 500ms);
        QVERIFY(manager.hasActiveBackup());

        manager.cancelPendingRestore();
        QVERIFY(!manager.hasActiveBackup());
    }

    void testDestructorAutoRestoreSafelyHandlesMissingTool() {
        // When manager is deleted while an active backup is pending,
        // the destructor invokes restore() safely without crashing.
        {
            TestableClipboardManager scopedManager;
            scopedManager.mockToolsExist = false;
            scopedManager.scheduleRestore(u"Must restore on exit"_s, true, 500ms);
            QVERIFY(scopedManager.hasActiveBackup());
        }
        // Exited cleanly without crash
        QVERIFY(true);
    }
};

QTEST_GUILESS_MAIN(TestClipboardManager)
#include "test_clipboard_manager.moc"
