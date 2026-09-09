#include "AudioRecorder.h"
#include "DictationCoordinator.h"

#include "CompositeSttEngine.h"
#include "LinuxNotificationService.h"
#include "NotificationPresenter.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class FakeNotificationBackend : public LinuxNotificationService {
    Q_OBJECT

public:
    explicit FakeNotificationBackend(QObject* parent = nullptr)
        : LinuxNotificationService(parent) { }

    uint showNotification(const QString& title, const QString& body, Urgency urgency, const QString& icon,
                          int timeoutMs, const QStringList& actions, uint replacesId) override {
        Q_UNUSED(timeoutMs);
        if (!isEnabled()) {
            return 0;
        }
        m_lastTitle = title;
        m_lastBody = body;
        m_lastUrgency = urgency;
        m_lastIcon = icon;
        m_lastActions = actions;
        m_lastReplacesId = replacesId;
        m_callCount++;
        return ++m_nextId;
    }

    uint showCategoryNotification(const QString& categoryKey, const QString& title, const QString& body,
                                  Urgency urgency, const QString& icon, int timeoutMs,
                                  const QStringList& actions) override {
        m_lastCategory = categoryKey;
        return showNotification(title, body, urgency, icon, timeoutMs, actions, 0);
    }

    void clearCategory(const QString& categoryKey) override { m_clearedCategories.append(categoryKey); }

    uint m_nextId = 100;
    int m_callCount = 0;
    QString m_lastTitle;
    QString m_lastBody;
    QString m_lastCategory;
    Urgency m_lastUrgency = Urgency::Normal;
    QString m_lastIcon;
    QStringList m_lastActions;
    uint m_lastReplacesId = 0;
    QStringList m_clearedCategories;
};

class FakeAudioRecorderDevice : public AudioRecorder {
    Q_OBJECT

public:
    explicit FakeAudioRecorderDevice(QObject* parent = nullptr)
        : AudioRecorder(parent) {
        setHasAudioInputDevice(true);
    }

    void triggerDeviceDisconnected(const QString& deviceName, const QString& fallbackDeviceName) {
        emit audioInputDeviceDisconnected(deviceName, fallbackDeviceName);
    }

    void setMockHasAudioInputDevice(bool hasDevice) { setHasAudioInputDevice(hasDevice); }
};

class TestNotificationPresenter : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testTranscriptionErrorNotification();
    void testMaxDurationWarningNotification();
    void testLlmFallbackNotification();
    void testCategoryClearedOnTranscriptionFinished();
    void testMicrophoneDisconnectedNotification();
    void testNoMicrophoneNotificationAndClear();
    void testFailoverNotifications();
    void testActionInvokedRouting();

private:
    NotificationPresenter* m_presenter = nullptr;
    FakeNotificationBackend* m_service = nullptr;
    DictationCoordinator* m_coordinator = nullptr;
    FakeAudioRecorderDevice* m_recorder = nullptr;
    CompositeSttEngine* m_sttEngine = nullptr;
};

void TestNotificationPresenter::initTestCase() {
    QCoreApplication::setOrganizationName(u"QTranscribeTestOrg"_s);
    QCoreApplication::setApplicationName(u"QTranscribeTestApp"_s);
}

void TestNotificationPresenter::cleanupTestCase() { }

void TestNotificationPresenter::init() {
    m_presenter = new NotificationPresenter(this);
    m_service = new FakeNotificationBackend(this);
    m_coordinator = new DictationCoordinator(this);
    m_recorder = new FakeAudioRecorderDevice(this);
    m_sttEngine = new CompositeSttEngine(this);

    m_presenter->setNotificationService(m_service);
    m_presenter->setDictationCoordinator(m_coordinator);
    m_presenter->setAudioRecorder(m_recorder);
    m_presenter->setCompositeSttEngine(m_sttEngine);
}

void TestNotificationPresenter::cleanup() {
    delete m_presenter;
    delete m_service;
    delete m_coordinator;
    delete m_recorder;
    delete m_sttEngine;
}

void TestNotificationPresenter::testTranscriptionErrorNotification() {
    emit m_coordinator->errorOccurred(u"Engine failed"_s);

    QCOMPARE(m_service->m_callCount, 1);
    QCOMPARE(m_service->m_lastCategory, u"transcription"_s);
    QCOMPARE(m_service->m_lastTitle, u"Transcription Failed"_s);
    QCOMPARE(m_service->m_lastBody, u"Engine failed"_s);
    QCOMPARE(m_service->m_lastUrgency, LinuxNotificationService::Urgency::Critical);
}

void TestNotificationPresenter::testMaxDurationWarningNotification() {
    emit m_coordinator->maxDurationWarningTriggered();

    QCOMPARE(m_service->m_callCount, 1);
    QCOMPARE(m_service->m_lastCategory, u"recording_limit"_s);
    QCOMPARE(m_service->m_lastTitle, u"Recording Limit Reached"_s);
    QCOMPARE(m_service->m_lastUrgency, LinuxNotificationService::Urgency::Normal);
}

void TestNotificationPresenter::testLlmFallbackNotification() {
    emit m_coordinator->llmFallbackWarningTriggered(u"Timeout error"_s);

    QCOMPARE(m_service->m_callCount, 1);
    QCOMPARE(m_service->m_lastCategory, u"llm_fallback"_s);
    QCOMPARE(m_service->m_lastTitle, u"AI Formatting Unavailable"_s);
    QVERIFY(m_service->m_lastBody.contains(u"Timeout error"_s));
    QCOMPARE(m_service->m_lastUrgency, LinuxNotificationService::Urgency::Low);
}

void TestNotificationPresenter::testCategoryClearedOnTranscriptionFinished() {
    emit m_coordinator->transcriptionFinished(u"Hello world"_s);
    QVERIFY(m_service->m_clearedCategories.contains(u"transcription"_s));
}

void TestNotificationPresenter::testMicrophoneDisconnectedNotification() {
    m_recorder->triggerDeviceDisconnected(u"JBL Headset"_s, u"Webcam Mic"_s);

    QCOMPARE(m_service->m_callCount, 1);
    QCOMPARE(m_service->m_lastCategory, u"microphone"_s);
    QCOMPARE(m_service->m_lastTitle, u"Microphone Disconnected"_s);
    QVERIFY(m_service->m_lastBody.contains(u"JBL Headset"_s));
    QVERIFY(m_service->m_lastBody.contains(u"Webcam Mic"_s));
}

void TestNotificationPresenter::testNoMicrophoneNotificationAndClear() {
    m_recorder->setMockHasAudioInputDevice(false);

    QCOMPARE(m_service->m_callCount, 1);
    QCOMPARE(m_service->m_lastCategory, u"microphone"_s);
    QCOMPARE(m_service->m_lastTitle, u"No Microphone Detected"_s);

    m_recorder->setMockHasAudioInputDevice(true);
    QVERIFY(m_service->m_clearedCategories.contains(u"microphone"_s));
}

void TestNotificationPresenter::testFailoverNotifications() {
    emit m_sttEngine->failoverTriggered();
    QCOMPARE(m_service->m_lastTitle, u"QtScribe: Switched to Offline Dictation"_s);

    emit m_sttEngine->onlineRestored();
    QCOMPARE(m_service->m_lastTitle, u"QtScribe: Internet Connection Restored"_s);

    emit m_sttEngine->offlineModelRequired();
    QCOMPARE(m_service->m_lastTitle, u"QtScribe: Offline Model Required"_s);

    emit m_sttEngine->networkDisconnectedDuringSession();
    QCOMPARE(m_service->m_lastTitle, u"QtScribe: Network Disconnected"_s);
}

void TestNotificationPresenter::testActionInvokedRouting() {
    QSignalSpy showSpy(m_presenter, &NotificationPresenter::requestShowWindow);
    QSignalSpy navSpy(m_presenter, &NotificationPresenter::requestNavigate);
    QSignalSpy quitSpy(m_presenter, &NotificationPresenter::requestQuitApp);

    m_service->onActionInvoked(101, u"default"_s);
    QCOMPARE(showSpy.count(), 1);

    m_service->onActionInvoked(102, u"settings"_s);
    QCOMPARE(navSpy.count(), 1);
    QCOMPARE(navSpy.last().at(0).toString(), u"system"_s);

    m_service->onActionInvoked(103, u"cloud"_s);
    QCOMPARE(navSpy.count(), 2);
    QCOMPARE(navSpy.last().at(0).toString(), u"cloud"_s);

    m_service->onActionInvoked(104, u"offline"_s);
    QCOMPARE(navSpy.count(), 3);
    QCOMPARE(navSpy.last().at(0).toString(), u"offline"_s);

    m_service->onActionInvoked(105, u"quit"_s);
    QCOMPARE(quitSpy.count(), 1);
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    TestNotificationPresenter test;
    const int result = QTest::qExec(&test, argc, argv);
    std::quick_exit(result);
}

#include "test_notification_presenter.moc"
