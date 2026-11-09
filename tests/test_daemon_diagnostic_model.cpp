#include "DaemonDiagnosticModel.h"

#include "DaemonClient.h"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class TestDaemonDiagnosticModel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testInitialProperties();
    void testClipboardWarningFlow();
    void testSettingsPersistence();
    void testTestTypingDelegation();
    void testDaemonClientCanRecordOffline();
    void testDaemonClientFocusCaching();
    void testDaemonClientHonorsDisableRemoteAttach();

private:
    DaemonDiagnosticModel* m_model = nullptr;
};

void TestDaemonDiagnosticModel::initTestCase() {
    qputenv("QTSCRIBE_DISABLE_REMOTE_ATTACH", "1");
    QCoreApplication::setOrganizationName(u"QTranscribeTestOrg"_s);
    QCoreApplication::setApplicationName(u"QTranscribeTestApp_DaemonDiagnosticModel"_s);
    QSettings settings;
    settings.clear();
}

void TestDaemonDiagnosticModel::cleanupTestCase() {
    QSettings settings;
    settings.clear();
}

void TestDaemonDiagnosticModel::init() {
    QSettings settings;
    settings.clear();

    m_model = new DaemonDiagnosticModel(this);
}

void TestDaemonDiagnosticModel::cleanup() {
    delete m_model;
    m_model = nullptr;
}

void TestDaemonDiagnosticModel::testInitialProperties() {
    QVERIFY(!m_model->isConnected());
    QVERIFY(!m_model->hasFatalError());
    QVERIFY(m_model->fatalErrorMessage().isEmpty());
    QVERIFY(m_model->lastError().isEmpty());
}

void TestDaemonDiagnosticModel::testClipboardWarningFlow() {
    QVERIFY(!m_model->clipboardBannerDismissed());
    QSignalSpy bannerSpy(m_model, &DaemonDiagnosticModel::clipboardBannerDismissedChanged);
    m_model->setClipboardBannerDismissed(true);
    QVERIFY(m_model->clipboardBannerDismissed());
    QCOMPARE(bannerSpy.count(), 1);

    QVERIFY(!m_model->clipboardWarningAcknowledged());
    QSignalSpy ackSpy(m_model, &DaemonDiagnosticModel::clipboardWarningAcknowledgedChanged);
    QSignalSpy reqSpy(m_model, &DaemonDiagnosticModel::clipboardWarningRequiredChanged);

    m_model->setClipboardWarningAcknowledged(true);
    QVERIFY(m_model->clipboardWarningAcknowledged());
    QCOMPARE(ackSpy.count(), 1);
    QCOMPARE(reqSpy.count(), 1);
    QVERIFY(!m_model->clipboardWarningRequired());
}

void TestDaemonDiagnosticModel::testSettingsPersistence() {
    QSignalSpy delaySpy(m_model, &DaemonDiagnosticModel::injectionDelayChanged);
    m_model->setInjectionDelay(400);
    QCOMPARE(m_model->injectionDelay(), 400);
    QCOMPARE(delaySpy.count(), 1);

    QSettings settings;
    QCOMPARE(settings.value(u"Typing/PreInjectionDelayMs"_s).toInt(), 400);

    QSignalSpy clipSpy(m_model, &DaemonDiagnosticModel::preventClipboardHistoryChanged);
    m_model->setPreventClipboardHistory(false);
    QVERIFY(!m_model->preventClipboardHistory());
    QCOMPARE(clipSpy.count(), 1);
    QCOMPARE(settings.value(u"Clipboard/PreventHistory"_s).toBool(), false);
}

void TestDaemonDiagnosticModel::testTestTypingDelegation() {
    QVERIFY(m_model->testTextInjection());
    QVERIFY(m_model->testTextInjection(u"Custom Test Text"_s));
    QVERIFY(m_model->testTyping());
    QVERIFY(m_model->testTyping(u"Custom Test Text"_s));
}

void TestDaemonDiagnosticModel::testDaemonClientCanRecordOffline() {
    DaemonClient client;
    QVERIFY(!client.isConnected());
    QVERIFY(!client.canRecord());

    QSignalSpy canRecordSpy(&client, &DaemonClient::canRecordChanged);
    QMetaObject::invokeMethod(&client, "onServiceUnregistered", Q_ARG(QString, u"io.github.qtscribe.SpeechService"_s));
    QCOMPARE(canRecordSpy.count(), 1);
    QVERIFY(!client.canRecord());
}

void TestDaemonDiagnosticModel::testDaemonClientFocusCaching() {
    DaemonClient client;
    QVERIFY(!client.clientWindowFocused());

    QSignalSpy focusSpy(&client, &DaemonClient::clientWindowFocusedChanged);
    client.setClientWindowFocused(true);
    QVERIFY(client.clientWindowFocused());
    QCOMPARE(focusSpy.count(), 1);
}

void TestDaemonDiagnosticModel::testDaemonClientHonorsDisableRemoteAttach() {
    qputenv("QTSCRIBE_DISABLE_REMOTE_ATTACH", "1");
    DaemonClient client;
    QVERIFY(!client.isConnected());
    QVERIFY(!client.canRecord());
    QCOMPARE(client.statusMessage(), client.tr("Background service offline"));
}

QTEST_GUILESS_MAIN(TestDaemonDiagnosticModel)
#include "test_daemon_diagnostic_model.moc"
