#include "DBusService.h"
#include "DaemonDiagnosticModel.h"
#include "DictationCoordinator.h"
#include "GroqLlmClient.h"
#include "GroqSttClient.h"
#include "LoggingCategories.h"

#include "AudioFeedbackPlayer.h"
#include "CloudProviderModel.h"
#include "DaemonClient.h"
#include "GeminiSttClient.h"
#include "GuiWhisperClient.h"
#include "LinuxNotificationService.h"
#include "WhisperModelManager.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTimer>
#include <QtQml/QQmlExtensionPlugin>

#include <array>

Q_IMPORT_QML_PLUGIN(QtScribePlugin)

using namespace Qt::StringLiterals;

#ifndef QTSCRIBE_VERSION
#define QTSCRIBE_VERSION "0.0.0-dev"
#endif

class SettingsGuiService : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.qtscribe.SettingsGui")

public:
    explicit SettingsGuiService(QObject* parent = nullptr)
        : QObject(parent) { }

public slots:
    void activateWindow() {
        if (auto* client = DaemonClient::instance()) {
            client->showWindow();
        }
    }

    void navigateToSection(const QString& section) {
        if (auto* client = DaemonClient::instance()) {
            client->showWindow();
            client->navigateToSection(section);
        }
    }
};

int main(int argc, char* argv[]) {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "wayland");
    }

    QGuiApplication app(argc, argv);
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(u"Fusion"_s);
        QQuickStyle::setFallbackStyle(u"Fusion"_s);
    }
    app.setApplicationName(u"QtScribe"_s);
    app.setApplicationVersion(u"" QTSCRIBE_VERSION ""_s);
    app.setOrganizationName(u"QtScribe"_s);
    app.setOrganizationDomain(u"io.github.qtscribe"_s);
    app.setDesktopFileName(u"io.github.qtscribe"_s);
    app.setQuitOnLastWindowClosed(true);

    QCommandLineParser parser;
    parser.setApplicationDescription(u"QtScribe — Wayland native Speech-to-Text application"_s);
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption daemonOption(u"daemon"_s, u"Launch the headless daemon (qtscribed)"_s);
    QCommandLineOption toggleOption({u"t"_s, u"toggle"_s}, u"Toggle speech-to-text recording"_s);
    QCommandLineOption startOption(u"start"_s, u"Start speech-to-text recording"_s);
    QCommandLineOption stopOption(u"stop"_s, u"Stop speech-to-text recording"_s);
    QCommandLineOption showOption({u"s"_s, u"show"_s}, u"Show and activate the main window"_s);
    QCommandLineOption quitOption({u"q"_s, u"quit"_s}, u"Quit the running application"_s);
    QCommandLineOption sectionOption(u"section"_s, u"Navigate to settings section on startup"_s, u"section"_s);

    parser.addOption(daemonOption);
    parser.addOption(toggleOption);
    parser.addOption(startOption);
    parser.addOption(stopOption);
    parser.addOption(showOption);
    parser.addOption(quitOption);
    parser.addOption(sectionOption);

    parser.process(app);

    const auto launchDaemon = [](const QStringList& args = {}) -> bool {
        const QString appDir = QCoreApplication::applicationDirPath();
        const QString daemonInAppDir = appDir + u"/qtscribed"_s;
        const QString daemonExec = QFile::exists(daemonInAppDir) ? daemonInAppDir : u"qtscribed"_s;
        qCDebug(lcSpeech) << "QtScribe: Starting background daemon:" << daemonExec << args;
        return QProcess::startDetached(daemonExec, args);
    };

    if (parser.isSet(daemonOption)) {
        return launchDaemon({}) ? 0 : 1;
    }

    if (parser.isSet(toggleOption)) {
        if (DBusService::sendRemoteCommand(u"toggleRecording"_s)) {
            return 0;
        }
        return launchDaemon({u"--toggle"_s}) ? 0 : 1;
    }
    if (parser.isSet(startOption)) {
        if (DBusService::sendRemoteCommand(u"startRecording"_s)) {
            return 0;
        }
        return launchDaemon({u"--start"_s}) ? 0 : 1;
    }
    if (parser.isSet(stopOption)) {
        return DBusService::sendRemoteCommand(u"stopRecording"_s) ? 0 : 1;
    }
    if (parser.isSet(quitOption)) {
        return DBusService::sendRemoteCommand(u"quitApp"_s) ? 0 : 1;
    }

    const QString kGuiService = u"io.github.qtscribe.SettingsGui"_s;
    const QString kGuiPath = u"/io/github/qtscribe/SettingsGui"_s;

    if (QDBusConnection::sessionBus().isConnected()) {
        auto* guiService = new SettingsGuiService(&app);
        QDBusConnection::sessionBus().registerObject(kGuiPath, guiService, QDBusConnection::ExportAllSlots);
        if (!QDBusConnection::sessionBus().registerService(kGuiService)) {
            QDBusConnection::sessionBus().unregisterObject(kGuiPath);
            QDBusInterface guiIface(kGuiService, kGuiPath, kGuiService, QDBusConnection::sessionBus());
            if (guiIface.isValid()) {
                if (parser.isSet(sectionOption)) {
                    guiIface.call(QDBus::NoBlock, u"navigateToSection"_s, parser.value(sectionOption));
                } else {
                    guiIface.call(QDBus::NoBlock, u"activateWindow"_s);
                }
            }
            return 0;
        }
    }

    if (!DBusService::isRemoteRunning()) {
        launchDaemon({});
    }

    QIcon appIcon;
    constexpr auto kIconSizes = std::to_array({16, 24, 32, 64, 128, 256, 512});
    for (int sz : kIconSizes) {
        appIcon.addFile(QString(u":/qt/qml/QtScribe/assets/speech-to-text-%1.png"_s).arg(sz), QSize(sz, sz));
    }
    app.setWindowIcon(appIcon);

    QQmlApplicationEngine engine;

    QObject::connect(&engine, &QQmlEngine::quit, []() { QCoreApplication::exit(0); });

    QObject::connect(&app, &QGuiApplication::aboutToQuit, []() {
        if (!DBusService::isRemoteRunning()) {
            DictationCoordinator::removeHistoryFile();
        }
        if (auto* client = DaemonClient::instance()) {
            client->setClientWindowFocused(false);
        }
    });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    DaemonClient::instance();

    engine.loadFromModule("QtScribe", "Main");

    auto* daemonClient = DaemonClient::instance();
    const auto connectReload = [daemonClient](auto* sender, auto signal) {
        if (sender && daemonClient) {
            QObject::connect(sender, signal, daemonClient, &DaemonClient::reloadSettings);
        }
    };

    if (auto* groqLlm = engine.singletonInstance<GroqLlmClient*>("QtScribe", "GroqLlmClient")) {
        connectReload(groqLlm, &GroqLlmClient::enabledChanged);
        connectReload(groqLlm, &GroqLlmClient::selectedModelChanged);
        connectReload(groqLlm, &GroqLlmClient::activePresetChanged);
        connectReload(groqLlm, &GroqLlmClient::customPromptChanged);
        connectReload(groqLlm, &GroqLlmClient::temperatureChanged);
    }

    if (auto* groqStt = engine.singletonInstance<GroqSttClient*>("QtScribe", "GroqSttClient")) {
        connectReload(groqStt, &GroqSttClient::selectedModelChanged);
        connectReload(groqStt, &GroqSttClient::languageChanged);
        connectReload(groqStt, &GroqSttClient::customPromptChanged);
    }

    if (auto* geminiStt = engine.singletonInstance<GeminiSttClient*>("QtScribe", "GeminiSttClient")) {
        connectReload(geminiStt, &GeminiSttClient::selectedModelChanged);
        connectReload(geminiStt, &GeminiSttClient::modeChanged);
        connectReload(geminiStt, &GeminiSttClient::customVocabularyChanged);
    }

    if (auto* whisperManager = engine.singletonInstance<WhisperModelManager*>("QtScribe", "WhisperModelManager")) {
        connectReload(whisperManager, &WhisperModelManager::selectedModelChanged);
        connectReload(whisperManager, &WhisperModelManager::modelStatusChanged);
        if (auto* whisperGui = GuiWhisperClient::instance()) {
            QObject::connect(whisperManager, &WhisperModelManager::selectedModelChanged, whisperGui,
                             &GuiWhisperClient::modelStatusChanged);
            QObject::connect(whisperManager, &WhisperModelManager::modelStatusChanged, whisperGui,
                             &GuiWhisperClient::modelStatusChanged);
        }
    }

    if (auto* audioPlayer = engine.singletonInstance<AudioFeedbackPlayer*>("QtScribe", "AudioFeedbackPlayer")) {
        connectReload(audioPlayer, &AudioFeedbackPlayer::soundEnabledChanged);
    }

    if (auto* notifService =
            engine.singletonInstance<LinuxNotificationService*>("QtScribe", "LinuxNotificationService")) {
        connectReload(notifService, &LinuxNotificationService::enabledChanged);
    }

    if (auto* diagModel = engine.singletonInstance<DaemonDiagnosticModel*>("QtScribe", "DaemonDiagnosticModel")) {
        connectReload(diagModel, &DaemonDiagnosticModel::preventClipboardHistoryChanged);
        connectReload(diagModel, &DaemonDiagnosticModel::injectionDelayChanged);
    }

    if (auto* cloudProvider = engine.singletonInstance<CloudProviderModel*>("QtScribe", "CloudProviderModel")) {
        connectReload(cloudProvider, &CloudProviderModel::activeProviderChanged);
    }

    if (parser.isSet(sectionOption)) {
        const QString section = parser.value(sectionOption);
        QTimer::singleShot(0, [section]() {
            if (auto* client = DaemonClient::instance()) {
                client->navigateToSection(section);
            }
        });
    }

    return app.exec();
}

#include "main.moc"
