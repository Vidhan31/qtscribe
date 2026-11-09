#include "DBusService.h"
#include "DictationCoordinator.h"
#include "LoggingCategories.h"

#include "ApplicationContext.h"
#include "SystemService.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QFile>
#include <QProcess>
#include <QTimer>

#include <chrono>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

#ifndef QTSCRIBE_VERSION
#define QTSCRIBE_VERSION "0.0.0-dev"
#endif

/**
 * @brief Application entry point for the headless QtScribe background daemon.
 *
 * Configures core application metadata, parses command line options, initializes
 * the application context (speech pipeline, models, audio, and shortcuts), and
 * exports the D-Bus interface for client communication.
 */
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(u"QtScribe"_s);
    QCoreApplication::setApplicationVersion(u"" QTSCRIBE_VERSION ""_s);
    QCoreApplication::setOrganizationName(u"QtScribe"_s);
    QCoreApplication::setOrganizationDomain(u"io.github.qtscribe"_s);

    QCommandLineParser parser;
    parser.setApplicationDescription(u"QtScribe Daemon — Headless Speech-to-Text service"_s);
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption toggleOption({u"t"_s, u"toggle"_s}, u"Toggle speech-to-text recording"_s);
    const QCommandLineOption startOption(u"start"_s, u"Start speech-to-text recording"_s);
    const QCommandLineOption stopOption(u"stop"_s, u"Stop speech-to-text recording"_s);
    const QCommandLineOption showOption({u"s"_s, u"show"_s}, u"Show and activate the settings window"_s);
    const QCommandLineOption quitOption({u"q"_s, u"quit"_s}, u"Quit the running daemon"_s);

    parser.addOption(toggleOption);
    parser.addOption(startOption);
    parser.addOption(stopOption);
    parser.addOption(showOption);
    parser.addOption(quitOption);

    parser.process(app);

    const auto command = [&]() -> QString {
        if (parser.isSet(toggleOption)) {
            return u"toggleRecording"_s;
        }
        if (parser.isSet(startOption)) {
            return u"startRecording"_s;
        }
        if (parser.isSet(stopOption)) {
            return u"stopRecording"_s;
        }
        if (parser.isSet(quitOption)) {
            return u"quitApp"_s;
        }
        if (parser.isSet(showOption)) {
            return u"showWindow"_s;
        }
        return {};
    }();

    if (!QDBusConnection::sessionBus().isConnected()) {
        qCCritical(lcSpeech) << "qtscribed: Failed to connect to D-Bus session bus. Exiting.";
        return 1;
    }

    if (!QDBusConnection::sessionBus().registerService(u"io.github.qtscribe.SpeechService"_s)) {
        if (!command.isEmpty()) {
            return DBusService::sendRemoteCommand(command) ? 0 : 1;
        }
        if (parser.isSet(showOption)) {
            return DBusService::sendRemoteCommand(u"showWindow"_s) ? 0 : 1;
        }
        qCWarning(lcSpeech) << "qtscribed: Another daemon instance is already active on D-Bus. Exiting.";
        return 0;
    }

    const auto launchGui = [](const QStringList& args = {}) {
        const QString kGuiService = u"io.github.qtscribe.SettingsGui"_s;
        auto* busIface = QDBusConnection::sessionBus().interface();
        if (busIface && busIface->isServiceRegistered(kGuiService)) {
            QDBusInterface guiIface(kGuiService, u"/io/github/qtscribe/SettingsGui"_s, kGuiService,
                                    QDBusConnection::sessionBus());
            if (guiIface.isValid()) {
                const int secIdx = args.indexOf(u"--section"_s);
                if (secIdx >= 0 && secIdx + 1 < args.size()) {
                    guiIface.call(QDBus::NoBlock, u"navigateToSection"_s, args.at(secIdx + 1));
                } else {
                    guiIface.call(QDBus::NoBlock, u"activateWindow"_s);
                }
                return;
            }
        }
        const QString appDir = QCoreApplication::applicationDirPath();
        const QString guiInAppDir = appDir + u"/qtscribe"_s;
        const QString guiExec = QFile::exists(guiInAppDir) ? guiInAppDir : u"qtscribe"_s;
        QProcess::startDetached(guiExec, args);
    };

    if (parser.isSet(showOption)) {
        launchGui({});
        return 0;
    }

    DictationCoordinator::removeHistoryFile();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &DictationCoordinator::removeHistoryFile);

    ApplicationContext appContext(&app);
    appContext.initializeHeadless(true);

    auto* systemService = appContext.systemService();
    if (systemService) {
        QObject::connect(systemService, &SystemService::requestQuitApp, &app, &QCoreApplication::quit);

        QObject::connect(systemService, &SystemService::requestShowWindow, [launchGui]() {
            qCDebug(lcSpeech) << "qtscribed: Launching QtScribe GUI on request";
            launchGui({});
        });

        QObject::connect(systemService, &SystemService::requestNavigate, [launchGui](const QString& section) {
            qCDebug(lcSpeech) << "qtscribed: Launching QtScribe GUI navigating to" << section;
            launchGui({u"--section"_s, section});
        });
    }

    auto* coordinator = appContext.dictationCoordinator();

    if (parser.isSet(toggleOption) || parser.isSet(startOption)) {
        QTimer::singleShot(200ms, [coordinator]() {
            if (coordinator) {
                coordinator->startRecording();
            }
        });
    }

    return QCoreApplication::exec();
}
