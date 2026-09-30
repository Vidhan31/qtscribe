#include "ClipboardManager.h"
#include "DaemonConnector.h"
#include "TextInjector.h"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

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

private:
    DaemonConnector* m_connector = nullptr;
    ClipboardManager* m_clipboard = nullptr;
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
    m_connector = new DaemonConnector(this);
    m_clipboard = new ClipboardManager(this);
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
}

void TestTextInjector::testPreInjectionDelayAndCancel() {
    m_injector->setInjectionDelay(500);
    QCOMPARE(m_injector->injectionDelay(), 500);

    QVERIFY(m_injector->inject(u"Delayed injection text"_s));
    m_injector->cancel();

    QVERIFY(m_injector->inject(u"Another text"_s));
    m_injector->cancel();
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

QTEST_GUILESS_MAIN(TestTextInjector)
#include "test_text_injector.moc"
