#include "DBusService.h"
#include "DictationCoordinator.h"

#include "ApplicationContext.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTimer>
#include <QtQml/QQmlExtensionPlugin>

#include <array>

Q_IMPORT_QML_PLUGIN(QtScribePlugin)

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

#ifndef QTSCRIBE_VERSION
#define QTSCRIBE_VERSION "0.0.0-dev"
#endif

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
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(u"QtScribe — Wayland native Speech-to-Text application"_s);
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption toggleOption({u"t"_s, u"toggle"_s}, u"Toggle speech-to-text recording"_s);
    QCommandLineOption startOption(u"start"_s, u"Start speech-to-text recording"_s);
    QCommandLineOption stopOption(u"stop"_s, u"Stop speech-to-text recording"_s);
    QCommandLineOption showOption({u"s"_s, u"show"_s}, u"Show and activate the main window"_s);
    QCommandLineOption quitOption({u"q"_s, u"quit"_s}, u"Quit the running application"_s);

    parser.addOption(toggleOption);
    parser.addOption(startOption);
    parser.addOption(stopOption);
    parser.addOption(showOption);
    parser.addOption(quitOption);

    parser.process(app);

    const auto command = [&]() -> QString {
        if (parser.isSet(toggleOption))
            return u"toggleRecording"_s;
        if (parser.isSet(startOption))
            return u"startRecording"_s;
        if (parser.isSet(stopOption))
            return u"stopRecording"_s;
        if (parser.isSet(quitOption))
            return u"quitApp"_s;
        return u"showWindow"_s;
    }();
    if (DBusService::sendRemoteCommand(command)) {
        return 0;
    }

    QIcon appIcon;
    constexpr auto kIconSizes = std::to_array({16, 24, 32, 64, 128, 256, 512});
    for (int sz : kIconSizes) {
        appIcon.addFile(QString(u":/qt/qml/QtScribe/assets/speech-to-text-%1.png"_s).arg(sz), QSize(sz, sz));
    }
    app.setWindowIcon(appIcon);

    QQmlApplicationEngine engine;

    QObject::connect(&engine, &QQmlEngine::quit, []() { QCoreApplication::exit(0); });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    ApplicationContext appContext(&app);
    appContext.initialize(engine);

    auto* coordinator = appContext.dictationCoordinator();
    if (coordinator) {
        QObject::connect(coordinator, &DictationCoordinator::requestQuitApp, []() { QCoreApplication::exit(0); });
    }

    engine.loadFromModule("QtScribe", "Main");

    if (parser.isSet(toggleOption) || parser.isSet(startOption)) {
        QTimer::singleShot(200ms, [coordinator]() {
            if (coordinator) {
                coordinator->startRecording();
            }
        });
    }

    if (parser.isSet(showOption)) {
        QTimer::singleShot(100ms, [coordinator]() {
            if (coordinator) {
                coordinator->showWindow();
            }
        });
    }

    return app.exec();
}
