#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include "LinuxNotificationService.h"

using namespace Qt::StringLiterals;

class FakeNotificationService : public LinuxNotificationService {
    Q_OBJECT

public:
    explicit FakeNotificationService(QObject* parent = nullptr)
        : LinuxNotificationService(parent) { }

    uint showNotification(const QString& title, const QString& body, Urgency urgency = Urgency::Normal,
                          const QString& icon = QStringLiteral("dialog-information"), int timeoutMs = 5000,
                          const QStringList& actions = {}, uint replacesId = 0) override {
        Q_UNUSED(timeoutMs);
        if (!isEnabled()) {
            return 0;
        }
        m_lastTitle = title;
        m_lastBody = body;
        m_lastUrgency = urgency;
        m_lastIcon = icon;
        m_lastTimeoutMs = timeoutMs;
        m_lastActions = actions;
        m_lastReplacesId = replacesId;
        m_callCount++;
        return ++m_nextId;
    }

    uint m_nextId = 100;
    int m_callCount = 0;
    QString m_lastTitle;
    QString m_lastBody;
    Urgency m_lastUrgency = Urgency::Normal;
    QString m_lastIcon;
    int m_lastTimeoutMs = 0;
    QStringList m_lastActions;
    uint m_lastReplacesId = 0;
};

class TestLinuxNotificationService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testEnabledProperty();
    void testShowCategoryNotificationReplacement();
    void testNotificationClosedClearsCategory();
    void testActionInvokedSignal();
};

void TestLinuxNotificationService::initTestCase() {
    QCoreApplication::setOrganizationName(u"QTranscribeTestOrg"_s);
    QCoreApplication::setApplicationName(u"QTranscribeTestApp"_s);
}

void TestLinuxNotificationService::testEnabledProperty() {
    FakeNotificationService service;
    service.setEnabled(true);
    QVERIFY(service.isEnabled());

    QSignalSpy spy(&service, &LinuxNotificationService::enabledChanged);
    service.setEnabled(false);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!service.isEnabled());

    const uint id = service.showNotification(u"Test"_s, u"Body"_s);
    QCOMPARE(id, 0u);
    QCOMPARE(service.m_callCount, 0);

    service.setEnabled(true);
    QCOMPARE(spy.count(), 2);
    QVERIFY(service.isEnabled());
}

void TestLinuxNotificationService::testShowCategoryNotificationReplacement() {
    FakeNotificationService service;
    service.setEnabled(true);

    const uint firstId = service.showCategoryNotification(
        u"transcription"_s, u"Title 1"_s, u"Body 1"_s, LinuxNotificationService::Urgency::Critical,
        u"dialog-error"_s, 5000, {u"default"_s, u"Open"_s});

    QCOMPARE(firstId, 101u);
    QCOMPARE(service.m_lastReplacesId, 0u);
    QCOMPARE(service.m_lastTitle, u"Title 1"_s);
    QCOMPARE(service.m_lastUrgency, LinuxNotificationService::Urgency::Critical);
    QCOMPARE(service.m_lastActions, QStringList({u"default"_s, u"Open"_s}));

    const uint secondId = service.showCategoryNotification(
        u"transcription"_s, u"Title 2"_s, u"Body 2"_s, LinuxNotificationService::Urgency::Normal,
        u"dialog-warning"_s, 5000, {u"default"_s, u"Open"_s});

    QCOMPARE(secondId, 102u);
    QCOMPARE(service.m_lastReplacesId, 101u);
    QCOMPARE(service.m_lastTitle, u"Title 2"_s);

    service.clearCategory(u"transcription"_s);

    const uint thirdId = service.showCategoryNotification(
        u"transcription"_s, u"Title 3"_s, u"Body 3"_s);

    QCOMPARE(thirdId, 103u);
    QCOMPARE(service.m_lastReplacesId, 0u);
}

void TestLinuxNotificationService::testNotificationClosedClearsCategory() {
    FakeNotificationService service;
    service.setEnabled(true);

    const uint id = service.showCategoryNotification(u"download"_s, u"Downloading"_s, u"Model"_s);
    QCOMPARE(id, 101u);

    QSignalSpy closeSpy(&service, &LinuxNotificationService::notificationClosed);
    service.onNotificationClosed(id, 2);
    QCOMPARE(closeSpy.count(), 1);

    const uint newId = service.showCategoryNotification(u"download"_s, u"Finished"_s, u"Done"_s);
    QCOMPARE(newId, 102u);
    QCOMPARE(service.m_lastReplacesId, 0u);
}

void TestLinuxNotificationService::testActionInvokedSignal() {
    FakeNotificationService service;
    QSignalSpy actionSpy(&service, &LinuxNotificationService::actionInvoked);

    service.onActionInvoked(101, u"settings"_s);
    QCOMPARE(actionSpy.count(), 1);
    QCOMPARE(actionSpy.first().at(0).toUInt(), 101u);
    QCOMPARE(actionSpy.first().at(1).toString(), u"settings"_s);
}

QTEST_GUILESS_MAIN(TestLinuxNotificationService)
#include "test_linux_notification_service.moc"
