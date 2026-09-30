#include "AbstractTextInjector.h"
#include "DaemonConnector.h"
#include "DaemonDiagnosticModel.h"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class MockInjector : public AbstractTextInjector {
    Q_OBJECT

public:
    explicit MockInjector(QObject* parent = nullptr)
        : AbstractTextInjector(parent) { }

    bool inject(const QString& text) override {
        m_lastInjectedText = text;
        m_injectCount++;
        return true;
    }

    void cancel() override { m_cancelCount++; }

    QString m_lastInjectedText;
    int m_injectCount = 0;
    int m_cancelCount = 0;
};

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

private:
    DaemonConnector* m_connector = nullptr;
    MockInjector* m_injector = nullptr;
    DaemonDiagnosticModel* m_model = nullptr;
};

void TestDaemonDiagnosticModel::initTestCase() {
    QCoreApplication::setOrganizationName(u"QTranscribeTestOrg"_s);
    QCoreApplication::setApplicationName(u"QTranscribeTestApp"_s);
}

void TestDaemonDiagnosticModel::cleanupTestCase() {
    QSettings settings;
    settings.clear();
}

void TestDaemonDiagnosticModel::init() {
    m_connector = new DaemonConnector(this);
    m_injector = new MockInjector(this);
    m_model = new DaemonDiagnosticModel(this);
    m_model->setDaemonConnector(m_connector);
    m_model->setTextInjector(m_injector);
}

void TestDaemonDiagnosticModel::cleanup() {
    delete m_model;
    delete m_injector;
    delete m_connector;
    m_model = nullptr;
    m_injector = nullptr;
    m_connector = nullptr;
}

void TestDaemonDiagnosticModel::testInitialProperties() {
    QVERIFY(m_model->daemonConnector() == m_connector);
    QVERIFY(m_model->textInjector() == m_injector);
    QCOMPARE(m_model->isConnected(), m_connector->isConnected());
    QCOMPARE(m_model->hasFatalError(), m_connector->hasFatalError());
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
    QVERIFY(m_model->testTyping());
    QCOMPARE(m_injector->m_injectCount, 1);
    QCOMPARE(m_injector->m_lastInjectedText, u" [QtScribe Test] "_s);

    QVERIFY(m_model->testTyping(u"Custom Test Text"_s));
    QCOMPARE(m_injector->m_injectCount, 2);
    QCOMPARE(m_injector->m_lastInjectedText, u"Custom Test Text"_s);
}

QTEST_GUILESS_MAIN(TestDaemonDiagnosticModel)
#include "test_daemon_diagnostic_model.moc"
