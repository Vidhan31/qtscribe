#include "ClipboardManager.h"
#include "DaemonConnector.h"
#include "TextInjector.h"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class MockDaemonConnector : public DaemonConnector {
    Q_OBJECT
public:
    explicit MockDaemonConnector(QObject* parent = nullptr) : DaemonConnector(parent) {}

    bool mockConnected = false;
    bool mockConnectSuccess = true;
    bool mockCommandSuccess = true;
    int connectCalls = 0;
    int sendCommandCalls = 0;
    keyinjectord::Opcode lastOpcode = keyinjectord::Opcode::Ping;

    bool isConnected() const override { return mockConnected; }

    bool connectToServer() override {
        connectCalls++;
        if (mockConnectSuccess) {
            mockConnected = true;
            return true;
        }
        return false;
    }

    bool sendCommand(keyinjectord::Opcode opcode, int timeoutMs = 2000) override {
        Q_UNUSED(timeoutMs);
        sendCommandCalls++;
        lastOpcode = opcode;
        return mockCommandSuccess;
    }
};

class MockClipboardManager : public ClipboardManager {
    Q_OBJECT
public:
    explicit MockClipboardManager(QObject* parent = nullptr) : ClipboardManager(parent) {}

    QString mockBackup = u"Original clipboard text"_s;
    QString lastSetText;
    bool lastSensitive = false;
    bool setTextSuccess = true;
    int backupCalls = 0;
    int setTextCalls = 0;
    int restoreCalls = 0;
    int scheduleRestoreCalls = 0;
    QString lastRestoredText;

    QString backupText() const override {
        const_cast<MockClipboardManager*>(this)->backupCalls++;
        return mockBackup;
    }

    bool setText(const QString& text, bool sensitive = true) override {
        setTextCalls++;
        lastSetText = text;
        lastSensitive = sensitive;
        return setTextSuccess;
    }

    void restore(const QString& backup, bool hadContent) override {
        Q_UNUSED(hadContent);
        restoreCalls++;
        lastRestoredText = backup;
    }

    void scheduleRestore(const QString& backupText, bool hadContent,
                          std::chrono::milliseconds delay = std::chrono::milliseconds(800)) override {
        Q_UNUSED(hadContent);
        Q_UNUSED(delay);
        scheduleRestoreCalls++;
        lastRestoredText = backupText;
    }
};

class TestTextInjector : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testEmptyTextIsNoOp();
    void testPreInjectionDelayAndCancel();
    void testSettingsPersistence();
    void testImmediateInjectionSuccess();
    void testFailedDaemonConnectionReturnsFalse();
    void testPasteCommandFailureTriggersImmediateRestore();
    void testDelayedInjectionTimerFires();

private:
    MockDaemonConnector* m_connector = nullptr;
    MockClipboardManager* m_clipboard = nullptr;
    TextInjector* m_injector = nullptr;
};

void TestTextInjector::initTestCase() {
    QCoreApplication::setOrganizationName(u"QTranscribeTestOrg"_s);
    QCoreApplication::setApplicationName(u"QTranscribeTestApp"_s);
}

void TestTextInjector::cleanupTestCase() {
    QSettings settings;
    settings.clear();
}

void TestTextInjector::init() {
    m_connector = new MockDaemonConnector(this);
    m_clipboard = new MockClipboardManager(this);
    m_injector = new TextInjector(m_connector, m_clipboard, this);
}

void TestTextInjector::cleanup() {
    delete m_injector;
    delete m_clipboard;
    delete m_connector;
    m_injector = nullptr;
    m_clipboard = nullptr;
    m_connector = nullptr;
}

void TestTextInjector::testEmptyTextIsNoOp() {
    QVERIFY(m_injector->inject(QString()));
    QVERIFY(m_injector->inject(u""_s));
    QCOMPARE(m_connector->sendCommandCalls, 0);
}

void TestTextInjector::testPreInjectionDelayAndCancel() {
    m_injector->setInjectionDelay(500);
    QCOMPARE(m_injector->injectionDelay(), 500);

    QVERIFY(m_injector->inject(u"Delayed injection text"_s));
    m_injector->cancel();

    QVERIFY(m_injector->inject(u"Another text"_s));
    m_injector->cancel();
    QCOMPARE(m_connector->sendCommandCalls, 0);
}

void TestTextInjector::testSettingsPersistence() {
    m_injector->setInjectionDelay(350);
    QCOMPARE(m_injector->injectionDelay(), 350);

    QSettings settings;
    QCOMPARE(settings.value(u"Typing/PreInjectionDelayMs"_s).toInt(), 350);

    m_injector->setPreventClipboardHistory(false);
    QVERIFY(!m_injector->preventClipboardHistory());
    QCOMPARE(settings.value(u"Clipboard/PreventHistory"_s).toBool(), false);

    m_injector->setPreventClipboardHistory(true);
    QVERIFY(m_injector->preventClipboardHistory());
    QCOMPARE(settings.value(u"Clipboard/PreventHistory"_s).toBool(), true);
}

void TestTextInjector::testImmediateInjectionSuccess() {
    m_injector->setInjectionDelay(0);
    m_connector->mockConnected = true;

    const bool ok = m_injector->inject(u"Hello Immediate World"_s);
    QVERIFY(ok);

    QCOMPARE(m_clipboard->setTextCalls, 1);
    QCOMPARE(m_clipboard->lastSetText, u"Hello Immediate World"_s);
    QVERIFY(m_clipboard->lastSensitive);

    QCOMPARE(m_connector->sendCommandCalls, 1);
    QCOMPARE(m_connector->lastOpcode, keyinjectord::Opcode::Paste);

    QCOMPARE(m_clipboard->scheduleRestoreCalls, 1);
    QCOMPARE(m_clipboard->lastRestoredText, u"Original clipboard text"_s);
}

void TestTextInjector::testFailedDaemonConnectionReturnsFalse() {
    m_injector->setInjectionDelay(0);
    m_connector->mockConnected = false;
    m_connector->mockConnectSuccess = false;

    const bool ok = m_injector->inject(u"Text without daemon"_s);
    QVERIFY(!ok);
    QCOMPARE(m_clipboard->setTextCalls, 0);
    QCOMPARE(m_connector->sendCommandCalls, 0);
}

void TestTextInjector::testPasteCommandFailureTriggersImmediateRestore() {
    m_injector->setInjectionDelay(0);
    m_connector->mockConnected = true;
    m_connector->mockCommandSuccess = false;

    const bool ok = m_injector->inject(u"Text with failing paste"_s);
    QVERIFY(!ok);

    QCOMPARE(m_clipboard->setTextCalls, 1);
    QCOMPARE(m_clipboard->restoreCalls, 1);
    QCOMPARE(m_clipboard->scheduleRestoreCalls, 0);
    QCOMPARE(m_clipboard->lastRestoredText, u"Original clipboard text"_s);
}

void TestTextInjector::testDelayedInjectionTimerFires() {
    m_injector->setInjectionDelay(20);
    m_connector->mockConnected = true;

    QVERIFY(m_injector->inject(u"Delayed typed text"_s));
    QCOMPARE(m_connector->sendCommandCalls, 0);

    QTRY_COMPARE(m_connector->sendCommandCalls, 1);
    QCOMPARE(m_clipboard->lastSetText, u"Delayed typed text"_s);
}

QTEST_GUILESS_MAIN(TestTextInjector)
#include "test_text_injector.moc"
