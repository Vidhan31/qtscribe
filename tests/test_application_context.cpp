#include "AudioRecorder.h"
#include "DaemonConnector.h"
#include "DaemonDiagnosticModel.h"
#include "DictationCoordinator.h"
#include "GroqSttClient.h"
#include "TextInjector.h"
#include "TranscriptionModel.h"

#include "AbstractLlmClient.h"
#include "ApiKeyStore.h"
#include "ApplicationContext.h"
#include "AudioFeedbackPlayer.h"
#include "CloudProviderModel.h"
#include "CloudSttRouter.h"
#include "CompositeSttEngine.h"
#include "DictationPadModel.h"
#include "GeminiSttClient.h"
#include "GlobalShortcutManager.h"
#include "NotificationPresenter.h"
#include "SystemHealthMonitor.h"
#include "WhisperModelManager.h"
#include "WhisperSttClient.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class TestApplicationContext : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testHeadlessInitialization();
    void testSubsystemWiringIntegrity();
    void testRepeatedInitializeIsNoOp();
};

void TestApplicationContext::initTestCase() {
    QCoreApplication::setOrganizationName(u"QTranscribeTestOrg"_s);
    QCoreApplication::setApplicationName(u"QTranscribeTestApp"_s);
}

void TestApplicationContext::testHeadlessInitialization() {
    ApplicationContext context;
    QVERIFY(!context.isInitialized());
    QVERIFY(context.dictationCoordinator() == nullptr);

    context.initializeHeadless();
    QVERIFY(context.isInitialized());

    QVERIFY(context.dictationCoordinator() != nullptr);
    QVERIFY(context.compositeSttEngine() != nullptr);
    QVERIFY(context.notificationPresenter() != nullptr);
    QVERIFY(context.dictationPadModel() != nullptr);
    QVERIFY(context.systemHealthMonitor() != nullptr);
    QVERIFY(context.audioFeedbackPlayer() != nullptr);
    QVERIFY(context.apiKeyStore() != nullptr);
    QVERIFY(context.whisperSttClient() != nullptr);
    QVERIFY(context.whisperModelManager() != nullptr);
    QVERIFY(context.groqSttClient() != nullptr);
    QVERIFY(context.geminiSttClient() != nullptr);
    QVERIFY(context.cloudSttRouter() != nullptr);
    QVERIFY(context.llmClient() != nullptr);
    QVERIFY(context.cloudProviderModel() != nullptr);
    QVERIFY(context.audioRecorder() != nullptr);
    QVERIFY(context.shortcutManager() != nullptr);
    QVERIFY(context.textInjector() != nullptr);
    QVERIFY(context.daemonConnector() != nullptr);
    QVERIFY(context.daemonDiagnosticModel() != nullptr);
    QVERIFY(context.historyModel() != nullptr);
}

void TestApplicationContext::testSubsystemWiringIntegrity() {
    ApplicationContext context;
    context.initializeHeadless();

    QCOMPARE(context.whisperSttClient()->modelManager(), context.whisperModelManager());
    QVERIFY(context.dictationCoordinator() != nullptr);
    QCOMPARE(context.dictationCoordinator()->sttEngine(), context.compositeSttEngine());
    QCOMPARE(context.notificationPresenter()->dictationCoordinator(), context.dictationCoordinator());
    QCOMPARE(context.notificationPresenter()->compositeSttEngine(), context.compositeSttEngine());
    QVERIFY(context.llmClient() != nullptr);
    QCOMPARE(context.cloudSttRouter()->cloudProviderModel(), context.cloudProviderModel());
    QVERIFY(context.cloudSttRouter()->activeClient() != nullptr);
    QCOMPARE(context.daemonDiagnosticModel()->daemonConnector(), context.daemonConnector());
    QCOMPARE(context.daemonDiagnosticModel()->textInjector(), context.textInjector());
}

void TestApplicationContext::testRepeatedInitializeIsNoOp() {
    ApplicationContext context;
    context.initializeHeadless();
    auto* initialCoordinator = context.dictationCoordinator();

    context.initializeHeadless();
    QCOMPARE(context.dictationCoordinator(), initialCoordinator);
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    TestApplicationContext test;
    const int result = QTest::qExec(&test, argc, argv);
    std::quick_exit(result);
}
#include "test_application_context.moc"
