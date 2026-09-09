#include "AudioRecorder.h"
#include "DictationCoordinator.h"

#include "StatusNotifierService.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class FakeAudioRecorder : public AudioRecorder {
    Q_OBJECT

public:
    explicit FakeAudioRecorder(QObject* parent = nullptr)
        : AudioRecorder(parent) {
        setHasAudioInputDevice(true);
    }

    void startRecording() override { setRecording(true); }

    void stopRecording() override {
        setRecording(false);
        emit recordingFinished(QByteArray("RIFFfakeWavData"));
    }

    void cancelRecording() override { setRecording(false); }
};

class TestStatusNotifier : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testRegistrationAndProperties();
    void testActivation();
    void testDBusMenuLayoutAndEvents();
    void testRecordingStateUpdates();

private:
    DictationCoordinator* m_coordinator = nullptr;
    FakeAudioRecorder* m_recorder = nullptr;
    StatusNotifierService* m_service = nullptr;
};

void TestStatusNotifier::initTestCase() {
    if (!QDBusConnection::sessionBus().isConnected()) {
        QSKIP("D-Bus session bus not available in test environment");
    }

    QCoreApplication::setOrganizationName(u"QTranscribeTestOrg"_s);
    QCoreApplication::setApplicationName(u"QTranscribeTestApp"_s);

    m_coordinator = new DictationCoordinator(this);
    m_recorder = new FakeAudioRecorder(this);

    m_coordinator->setAudioRecorder(m_recorder);
    m_coordinator->initialize();

    m_service = new StatusNotifierService(this);
    QVERIFY(m_service->registerController(m_coordinator));
}

void TestStatusNotifier::cleanupTestCase() {
    delete m_service;
    delete m_coordinator;
}

void TestStatusNotifier::testRegistrationAndProperties() {
    QVERIFY(m_service->isRegistered());
    QVERIFY(m_service->serviceName().startsWith(u"org.kde.StatusNotifierItem-"_s));

    QDBusInterface sniInterface(m_service->serviceName(), u"/StatusNotifierItem"_s, u"org.kde.StatusNotifierItem"_s,
                                QDBusConnection::sessionBus());

    QVERIFY(sniInterface.isValid());
    QCOMPARE(sniInterface.property("Category").toString(), u"ApplicationStatus"_s);
    QCOMPARE(sniInterface.property("Id").toString(), u"qtscribe"_s);
    QCOMPARE(sniInterface.property("Title").toString(), u"QtScribe"_s);
    QCOMPARE(sniInterface.property("Status").toString(), u"Active"_s);
    QCOMPARE(sniInterface.property("IconName").toString(), u"io.github.qtscribe"_s);
    QCOMPARE(sniInterface.property("Menu").value<QDBusObjectPath>().path(), u"/MenuBar"_s);
}

void TestStatusNotifier::testActivation() {
    QSignalSpy showSpy(m_coordinator, &DictationCoordinator::requestShowWindow);
    QVERIFY(showSpy.isValid());

    QDBusInterface sniInterface(m_service->serviceName(), u"/StatusNotifierItem"_s, u"org.kde.StatusNotifierItem"_s,
                                QDBusConnection::sessionBus());

    sniInterface.call(u"Activate"_s, 0, 0);
    QCOMPARE(showSpy.count(), 1);
}

void TestStatusNotifier::testDBusMenuLayoutAndEvents() {
    QDBusInterface menuInterface(m_service->serviceName(), u"/MenuBar"_s, u"com.canonical.dbusmenu"_s,
                                 QDBusConnection::sessionBus());

    QVERIFY(menuInterface.isValid());
    QCOMPARE(menuInterface.property("Status").toString(), u"normal"_s);
    QCOMPARE(menuInterface.property("Version").toUInt(), 3u);

    uint rev = m_service->revision();
    QVERIFY(rev >= 1);

    QSignalSpy showSpy(m_coordinator, &DictationCoordinator::requestShowWindow);
    menuInterface.call(u"Event"_s, 1, u"clicked"_s, QVariant::fromValue(QDBusVariant(0)), 0u);
    QCOMPARE(showSpy.count(), 1);

    QSignalSpy quitSpy(m_coordinator, &DictationCoordinator::requestQuitApp);
    menuInterface.call(u"Event"_s, 3, u"clicked"_s, QVariant::fromValue(QDBusVariant(0)), 0u);
    QCOMPARE(quitSpy.count(), 1);
}

void TestStatusNotifier::testRecordingStateUpdates() {
    QDBusInterface sniInterface(m_service->serviceName(), u"/StatusNotifierItem"_s, u"org.kde.StatusNotifierItem"_s,
                                QDBusConnection::sessionBus());

    QCOMPARE(sniInterface.property("IconName").toString(), u"io.github.qtscribe"_s);

    m_coordinator->startRecording();
    QCOMPARE(sniInterface.property("IconName").toString(), u"io.github.qtscribe"_s);

    m_coordinator->stopRecording();
    QCOMPARE(sniInterface.property("IconName").toString(), u"io.github.qtscribe"_s);
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    TestStatusNotifier test;
    const int result = QTest::qExec(&test, argc, argv);
    std::quick_exit(result);
}

#include "test_status_notifier.moc"
