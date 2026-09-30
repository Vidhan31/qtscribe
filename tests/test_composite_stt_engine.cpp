#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include "AbstractSttClient.h"
#include "CompositeSttEngine.h"
#include "SystemHealthMonitor.h"

using namespace Qt::StringLiterals;

class FakeSttBackend : public AbstractSttClient {
    Q_OBJECT

public:
    explicit FakeSttBackend(QObject* parent = nullptr)
        : AbstractSttClient(parent) { }

    void transcribe(const QByteArray& wavData) override {
        m_lastWav = wavData;
        m_transcribeCallCount++;
        m_isBusy = true;
        emit busyChanged();

        if (m_failNext) {
            m_isBusy = false;
            emit busyChanged();
            emit errorOccurred(m_simulatedError.isEmpty() ? u"Simulated failure"_s : m_simulatedError);
            return;
        }

        m_isBusy = false;
        emit busyChanged();
        emit transcriptionReady(m_simulatedText.isEmpty() ? u"Default transcribed text"_s : m_simulatedText);
    }

    void cancel() override {
        m_cancelCallCount++;
        m_isBusy = false;
        emit busyChanged();
    }

    bool isReady() const override { return m_isReady; }
    bool isBusy() const override { return m_isBusy; }

    int m_transcribeCallCount = 0;
    int m_cancelCallCount = 0;
    bool m_isReady = true;
    bool m_isBusy = false;
    bool m_failNext = false;
    QString m_simulatedError;
    QString m_simulatedText;
    QByteArray m_lastWav;
};

class TestCompositeSttEngine : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testInitialState();
    void testBackendSwitching();
    void testTranscriptionDelegation();
    void testAutoSwitchOfflineWhenNetworkLost();
    void testAutoSwitchOfflineDeferredDuringBusy();
    void testAutoSwitchOfflineRestoreWhenNetworkReturns();
    void testRetryLast();

private:
    CompositeSttEngine* m_engine = nullptr;
    FakeSttBackend* m_cloudClient = nullptr;
    FakeSttBackend* m_whisperClient = nullptr;
    SystemHealthMonitor* m_healthMonitor = nullptr;
};

void TestCompositeSttEngine::initTestCase() {
    QCoreApplication::setOrganizationName(u"QTranscribeTestOrg"_s);
    QCoreApplication::setApplicationName(u"QTranscribeTestApp"_s);
}

void TestCompositeSttEngine::cleanupTestCase() { }

void TestCompositeSttEngine::init() {
    m_engine = new CompositeSttEngine(this);
    m_cloudClient = new FakeSttBackend(this);
    m_whisperClient = new FakeSttBackend(this);
    m_healthMonitor = new SystemHealthMonitor(this);

    m_engine->setCloudClient(m_cloudClient);
    m_engine->setWhisperClient(m_whisperClient);
    m_engine->setHealthMonitor(m_healthMonitor);
    m_engine->activate();
}

void TestCompositeSttEngine::cleanup() {
    delete m_engine;
    delete m_cloudClient;
    delete m_whisperClient;
    delete m_healthMonitor;
}

void TestCompositeSttEngine::testInitialState() {
    QVERIFY(m_engine->cloudClient() == m_cloudClient);
    QVERIFY(m_engine->whisperClient() == m_whisperClient);
    QVERIFY(m_engine->healthMonitor() == m_healthMonitor);
    QVERIFY(m_engine->autoSwitchOffline());
    QVERIFY(!m_engine->isAutoSwitchedToOffline());
    QVERIFY(m_engine->isReady());
}

void TestCompositeSttEngine::testBackendSwitching() {
    QSignalSpy backendSpy(m_engine, &CompositeSttEngine::activeBackendChanged);

    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    QCOMPARE(m_engine->activeSttClient(), m_whisperClient);

    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);
    QCOMPARE(m_engine->activeSttClient(), m_cloudClient);
    QVERIFY(backendSpy.count() >= 1);
}

void TestCompositeSttEngine::testTranscriptionDelegation() {
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    m_cloudClient->m_simulatedText = u"Cloud output"_s;

    QSignalSpy textSpy(m_engine, &AbstractSttClient::transcriptionReady);
    const QByteArray wav("RIFFmockdata");
    m_engine->transcribe(wav);

    QCOMPARE(m_cloudClient->m_transcribeCallCount, 1);
    QCOMPARE(m_whisperClient->m_transcribeCallCount, 0);
    QCOMPARE(textSpy.count(), 1);
    QCOMPARE(textSpy.first().at(0).toString(), u"Cloud output"_s);

    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    m_whisperClient->m_simulatedText = u"Local output"_s;

    m_engine->transcribe(wav);
    QCOMPARE(m_whisperClient->m_transcribeCallCount, 1);
    QCOMPARE(textSpy.count(), 2);
    QCOMPARE(textSpy.at(1).at(0).toString(), u"Local output"_s);
}

void TestCompositeSttEngine::testAutoSwitchOfflineWhenNetworkLost() {
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);

    QSignalSpy failoverSpy(m_engine, &CompositeSttEngine::failoverTriggered);
    m_healthMonitor->setNetworkOnlineForTesting(false);

    QCOMPARE(failoverSpy.count(), 1);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    QVERIFY(m_engine->isAutoSwitchedToOffline());

    QSignalSpy restoreSpy(m_engine, &CompositeSttEngine::onlineRestored);
    m_healthMonitor->setNetworkOnlineForTesting(true);

    QCOMPARE(restoreSpy.count(), 1);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);
    QVERIFY(!m_engine->isAutoSwitchedToOffline());
}

void TestCompositeSttEngine::testAutoSwitchOfflineDeferredDuringBusy() {
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    m_cloudClient->m_failNext = false;
    m_cloudClient->m_isBusy = true;

    QSignalSpy sessionSpy(m_engine, &CompositeSttEngine::networkDisconnectedDuringSession);
    QSignalSpy failoverSpy(m_engine, &CompositeSttEngine::failoverTriggered);

    m_healthMonitor->setNetworkOnlineForTesting(false);
    QCOMPARE(sessionSpy.count(), 1);
    QCOMPARE(failoverSpy.count(), 0);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);

    m_cloudClient->m_isBusy = false;
    m_engine->checkPendingOfflineOrOnlineSwitch();

    QCOMPARE(failoverSpy.count(), 1);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    QVERIFY(m_engine->isAutoSwitchedToOffline());
}

void TestCompositeSttEngine::testAutoSwitchOfflineRestoreWhenNetworkReturns() {
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    m_healthMonitor->setNetworkOnlineForTesting(false);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::WhisperCpp);

    m_healthMonitor->setNetworkOnlineForTesting(true);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);
}

void TestCompositeSttEngine::testRetryLast() {
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    m_cloudClient->m_failNext = true;
    m_cloudClient->m_simulatedError = u"Server 500"_s;

    QSignalSpy errorSpy(m_engine, &AbstractSttClient::errorOccurred);
    m_engine->transcribe(QByteArray("audio"));
    QCOMPARE(errorSpy.count(), 1);

    m_cloudClient->m_failNext = false;
    m_cloudClient->m_simulatedText = u"Retried text"_s;
    QSignalSpy readySpy(m_engine, &AbstractSttClient::transcriptionReady);

    m_engine->retryLast();
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(readySpy.first().at(0).toString(), u"Retried text"_s);
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    TestCompositeSttEngine test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_composite_stt_engine.moc"
