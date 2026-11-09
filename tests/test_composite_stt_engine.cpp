#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

#include "AbstractCloudSttClient.h"
#include "AbstractOfflineSttClient.h"
#include "CompositeSttEngine.h"
#include "SettingsManager.h"
#include "SystemHealthMonitor.h"
#include "WhisperSttClient.h"

using namespace Qt::StringLiterals;

class FakeCloudBackend : public AbstractCloudSttClient {
    Q_OBJECT

public:
    explicit FakeCloudBackend(QObject* parent = nullptr)
        : AbstractCloudSttClient(parent) { }

    void transcribe(const QByteArray& wavData) override {
        m_lastWav = wavData;
        m_transcribeCallCount++;
        m_isBusy = true;
        emit busyChanged();

        if (m_manualCompletion) {
            return;
        }

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
    bool m_manualCompletion = false;
    QString m_simulatedError;
    QString m_simulatedText;
    QByteArray m_lastWav;
};

class FakeOfflineBackend : public AbstractOfflineSttClient {
    Q_OBJECT

public:
    explicit FakeOfflineBackend(QObject* parent = nullptr)
        : AbstractOfflineSttClient(parent) { }

    void transcribe(const QByteArray& wavData) override {
        m_lastWav = wavData;
        m_transcribeCallCount++;
        m_isBusy = true;
        emit busyChanged();

        if (m_manualCompletion) {
            return;
        }

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

    bool isModelInstalled() const override { return m_isModelInstalled; }
    bool isModelLoaded() const override { return m_isModelLoaded; }
    QString modelPath() const override { return m_modelPath; }
    QString loadedModelPath() const override { return m_loadedModelPath; }
    QString computeDevice() const override { return m_computeDevice; }

    void loadModel(const QString& customPath = QString()) override {
        m_loadModelCalled = true;
        m_loadedModelPath = customPath;
    }
    void unloadModel() override {
        m_unloadModelCalled = true;
        m_loadedModelPath.clear();
    }
    void checkModelStatus() override {
        m_checkModelStatusCalled = true;
    }

    int m_transcribeCallCount = 0;
    int m_cancelCallCount = 0;
    bool m_isReady = true;
    bool m_isBusy = false;
    bool m_failNext = false;
    bool m_manualCompletion = false;
    bool m_isModelInstalled = true;
    bool m_isModelLoaded = false;
    bool m_loadModelCalled = false;
    bool m_unloadModelCalled = false;
    bool m_checkModelStatusCalled = false;
    QString m_modelPath;
    QString m_loadedModelPath;
    QString m_computeDevice;
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
    void testLoadSettingsPreservesOfflineFallback();
    void testRetryLast();
    void testWhisperModelDelegation();
    void testBackendChangeCancelsInFlightTranscription();
    void testExplicitSameBackendClearsFallback();

private:
    CompositeSttEngine* m_engine = nullptr;
    FakeCloudBackend* m_cloudClient = nullptr;
    FakeOfflineBackend* m_whisperClient = nullptr;
    SystemHealthMonitor* m_healthMonitor = nullptr;
};

void TestCompositeSttEngine::initTestCase() {
    QCoreApplication::setOrganizationName(u"QTranscribeTestOrg"_s);
    QCoreApplication::setApplicationName(u"QTranscribeTestApp_CompositeSttEngine"_s);
    QSettings settings;
    settings.clear();
}

void TestCompositeSttEngine::cleanupTestCase() {
    QSettings settings;
    settings.clear();
}

void TestCompositeSttEngine::init() {
    m_engine = new CompositeSttEngine(this);
    m_cloudClient = new FakeCloudBackend(this);
    m_whisperClient = new FakeOfflineBackend(this);
    m_healthMonitor = new SystemHealthMonitor(this);
    m_healthMonitor->setNetworkOnlineForTesting(true);

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
    QCOMPARE(backendSpy.count(), 1);

    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);
    QCOMPARE(backendSpy.count(), 2);
}

void TestCompositeSttEngine::testTranscriptionDelegation() {
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    QCOMPARE(m_engine->activeSttClient(), m_cloudClient);

    QSignalSpy textSpy(m_engine, &AbstractSttClient::transcriptionReady);
    m_engine->transcribe(QByteArray("cloud audio"));

    QCOMPARE(m_cloudClient->m_transcribeCallCount, 1);
    QCOMPARE(m_whisperClient->m_transcribeCallCount, 0);
    QCOMPARE(textSpy.count(), 1);
    QCOMPARE(textSpy.first().at(0).toString(), u"Default transcribed text"_s);

    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    QCOMPARE(m_engine->activeSttClient(), m_whisperClient);

    m_whisperClient->m_simulatedText = u"Local output"_s;
    m_engine->transcribe(QByteArray("local audio"));

    QCOMPARE(m_whisperClient->m_transcribeCallCount, 1);
    QCOMPARE(textSpy.count(), 2);
    QCOMPARE(textSpy.last().at(0).toString(), u"Local output"_s);
}

void TestCompositeSttEngine::testAutoSwitchOfflineWhenNetworkLost() {
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);

    QSignalSpy failoverSpy(m_engine, &CompositeSttEngine::failoverTriggered);
    m_healthMonitor->setNetworkOnlineForTesting(false);

    QVERIFY(m_engine->isAutoSwitchedToOffline());
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    QCOMPARE(failoverSpy.count(), 1);

    QSignalSpy restoreSpy(m_engine, &CompositeSttEngine::onlineRestored);
    m_healthMonitor->setNetworkOnlineForTesting(true);

    QVERIFY(!m_engine->isAutoSwitchedToOffline());
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);
    QCOMPARE(restoreSpy.count(), 1);
}

void TestCompositeSttEngine::testAutoSwitchOfflineDeferredDuringBusy() {
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    m_cloudClient->m_isBusy = true;
    QVERIFY(m_engine->isBusy());

    QSignalSpy sessionSpy(m_engine, &CompositeSttEngine::networkDisconnectedDuringSession);
    QSignalSpy failoverSpy(m_engine, &CompositeSttEngine::failoverTriggered);

    m_healthMonitor->setNetworkOnlineForTesting(false);

    QCOMPARE(sessionSpy.count(), 1);
    QCOMPARE(failoverSpy.count(), 0);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);

    m_cloudClient->m_isBusy = false;
    m_engine->checkPendingOfflineOrOnlineSwitch();

    QVERIFY(m_engine->isAutoSwitchedToOffline());
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    QCOMPARE(failoverSpy.count(), 1);
}

void TestCompositeSttEngine::testAutoSwitchOfflineRestoreWhenNetworkReturns() {
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    m_healthMonitor->setNetworkOnlineForTesting(false);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::WhisperCpp);

    m_healthMonitor->setNetworkOnlineForTesting(true);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);
}

void TestCompositeSttEngine::testLoadSettingsPreservesOfflineFallback() {
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud, true);
    m_healthMonitor->setNetworkOnlineForTesting(false);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    QVERIFY(m_engine->isAutoSwitchedToOffline());

    m_engine->loadSettings();
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    QVERIFY(m_engine->isAutoSwitchedToOffline());

    m_healthMonitor->setNetworkOnlineForTesting(true);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);
}

void TestCompositeSttEngine::testRetryLast() {
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud);
    m_cloudClient->m_failNext = true;
    m_cloudClient->m_simulatedError = u"Mock error"_s;

    QSignalSpy errorSpy(m_engine, &AbstractSttClient::errorOccurred);
    m_engine->transcribe(QByteArray("test wav retry payload"));
    QCOMPARE(errorSpy.count(), 1);

    m_cloudClient->m_failNext = false;
    m_cloudClient->m_simulatedText = u"Retried text"_s;
    QSignalSpy readySpy(m_engine, &AbstractSttClient::transcriptionReady);

    m_engine->retryLast();
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(readySpy.first().at(0).toString(), u"Retried text"_s);
}

void TestCompositeSttEngine::testWhisperModelDelegation() {
    auto* whisper = new WhisperSttClient(this);
    m_engine->setWhisperClient(whisper);
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::WhisperCpp, false);
    m_engine->activate();

    m_engine->loadModel(u"/test/custom_path.bin"_s);
    QVERIFY(!whisper->isModelLoaded());

    m_engine->unloadModel();
    QVERIFY(!whisper->isModelLoaded());
    QVERIFY(whisper->isExplicitlyUnloaded());

    // Passive settings reload with unchanged offline backend must honor explicit unload
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::WhisperCpp, false);
    QVERIFY(whisper->isExplicitlyUnloaded());
}

void TestCompositeSttEngine::testBackendChangeCancelsInFlightTranscription() {
    auto* settings = new SettingsManager(this);
    settings->setBackend(SettingsManager::TranscriptionBackend::WhisperCpp);
    m_engine->setSettingsManager(settings);
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::WhisperCpp);

    m_whisperClient->m_manualCompletion = true;
    m_engine->transcribe(QByteArray("test wav payload"));

    QVERIFY(m_engine->isBusy());
    QCOMPARE(m_whisperClient->m_transcribeCallCount, 1);
    QCOMPARE(m_whisperClient->m_cancelCallCount, 0);

    // Change backend in SettingsManager while transcription is in flight
    settings->setBackend(SettingsManager::TranscriptionBackend::Cloud);

    QCOMPARE(m_whisperClient->m_cancelCallCount, 1);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::Cloud);
    QVERIFY(!m_engine->isBusy());
}

void TestCompositeSttEngine::testExplicitSameBackendClearsFallback() {
    auto* settings = new SettingsManager(this);
    m_engine->setSettingsManager(settings);
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::Cloud, true);
    m_healthMonitor->setNetworkOnlineForTesting(false);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    QVERIFY(m_engine->isAutoSwitchedToOffline());

    // Explicitly select WhisperCpp while it is already active via auto-switch
    m_engine->setActiveBackend(CompositeSttEngine::TranscriptionBackend::WhisperCpp, true);
    QVERIFY(!m_engine->isAutoSwitchedToOffline());
    QCOMPARE(settings->speechConfig().backend, SettingsManager::TranscriptionBackend::WhisperCpp);

    // When network returns, backend should remain WhisperCpp because user explicitly chose it
    m_healthMonitor->setNetworkOnlineForTesting(true);
    QCOMPARE(m_engine->activeBackend(), CompositeSttEngine::TranscriptionBackend::WhisperCpp);
    QVERIFY(!m_engine->isAutoSwitchedToOffline());
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    TestCompositeSttEngine test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_composite_stt_engine.moc"
