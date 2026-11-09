#include "DBusService.h"

#include "DictationCoordinator.h"
#include "LoggingCategories.h"

#include "ModelAdaptor.h"
#include "ModelService.h"
#include "SettingsAdaptor.h"
#include "SettingsService.h"
#include "SpeechAdaptor.h"
#include "SystemAdaptor.h"
#include "SystemService.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>

using namespace Qt::StringLiterals;

DBusService::DBusService(QObject* parent)
    : QObject(parent) { }

bool DBusService::sendRemoteCommand(const QString& method) {
    if (!QDBusConnection::sessionBus().isConnected()) {
        return false;
    }

    const QString targetInterface =
        (method == u"quitApp"_s || method == u"showWindow"_s || method == u"navigateToSection"_s ||
         method == u"testTextInjection"_s || method == u"restartTextInjection"_s || method == u"stopTextInjection"_s ||
         method == u"testDirectTyping"_s || method == u"restartDirectTyping"_s || method == u"stopDirectTyping"_s)
            ? kDbusSystemInterfaceName.toString()
            : kDbusInterfaceName.toString();

    QDBusInterface remoteService(kDbusServiceName.toString(), kDbusObjectPath.toString(), targetInterface,
                                 QDBusConnection::sessionBus());
    if (remoteService.isValid()) {
        const QDBusMessage reply = remoteService.call(method);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            // Fall back to primary interface if method was not on the specialized interface
            if (targetInterface != kDbusInterfaceName.toString()) {
                QDBusInterface fallbackService(kDbusServiceName.toString(), kDbusObjectPath.toString(),
                                               kDbusInterfaceName.toString(), QDBusConnection::sessionBus());
                if (fallbackService.isValid()) {
                    const QDBusMessage fallbackReply = fallbackService.call(method);
                    if (fallbackReply.type() != QDBusMessage::ErrorMessage) {
                        return true;
                    }
                }
            }
            qWarning() << "DBusService: Remote command" << method << "failed:" << reply.errorMessage();
            return false;
        }
        return true;
    }
    return false;
}

bool DBusService::isRemoteRunning() {
    if (!QDBusConnection::sessionBus().isConnected()) {
        return false;
    }
    auto* iface = QDBusConnection::sessionBus().interface();
    return iface && iface->isServiceRegistered(kDbusServiceName.toString());
}

bool DBusService::registerService() {
    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning() << "DBusService: Session bus not connected";
        return false;
    }

    auto* iface = QDBusConnection::sessionBus().interface();
    if (iface && iface->isServiceRegistered(kDbusServiceName.toString())) {
        m_registered = true;
        return true;
    }

    if (!QDBusConnection::sessionBus().registerService(kDbusServiceName.toString())) {
        qWarning() << "DBusService: Failed to register service" << kDbusServiceName;
        return false;
    }

    m_registered = true;
    return true;
}

bool DBusService::registerServices(DictationCoordinator* coordinator, ModelService* modelService,
                                   SystemService* systemService, SettingsService* settingsService) {
    if (!QDBusConnection::sessionBus().isConnected()) {
        return false;
    }

    if (!registerService()) {
        return false;
    }

    auto* rootObject = new QObject(this);
    if (coordinator) {
        new SpeechAdaptor(coordinator, rootObject);
    }
    if (modelService) {
        new ModelAdaptor(modelService, rootObject);
    }
    if (systemService) {
        new SystemAdaptor(systemService, rootObject);
    }
    if (settingsService) {
        new SettingsAdaptor(settingsService, rootObject);
    }

    const bool ok = QDBusConnection::sessionBus().registerObject(kDbusObjectPath.toString(), rootObject,
                                                                 QDBusConnection::ExportAdaptors);

    if (ok) {
        m_registered = true;
        qCDebug(lcSpeech) << "DBusService: Adaptors successfully exported on D-Bus at" << kDbusObjectPath;
    } else {
        qWarning() << "DBusService: Failed to register adaptors on D-Bus";
    }

    return ok;
}

bool DBusService::registerController(DictationCoordinator* coordinator) {
    return registerServices(coordinator, nullptr, nullptr);
}

bool DBusService::isRegistered() const {
    return m_registered;
}
