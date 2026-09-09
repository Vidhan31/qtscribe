#include "LinuxNotificationService.h"

#include "LoggingCategories.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QSettings>
#include <QVariantMap>

using namespace Qt::StringLiterals;

LinuxNotificationService::LinuxNotificationService(QObject* parent)
    : QObject(parent) {
    QSettings settings;
    m_enabled = settings.value(u"notifications/enabled"_s, true).toBool();

    if (QDBusConnection::sessionBus().isConnected()) {
        QDBusConnection::sessionBus().connect(u"org.freedesktop.Notifications"_s, u"/org/freedesktop/Notifications"_s,
                                              u"org.freedesktop.Notifications"_s, u"ActionInvoked"_s, this,
                                              SLOT(onActionInvoked(uint, QString)));

        QDBusConnection::sessionBus().connect(u"org.freedesktop.Notifications"_s, u"/org/freedesktop/Notifications"_s,
                                              u"org.freedesktop.Notifications"_s, u"NotificationClosed"_s, this,
                                              SLOT(onNotificationClosed(uint, uint)));
    }
}

bool LinuxNotificationService::isEnabled() const {
    return m_enabled;
}

void LinuxNotificationService::setEnabled(bool enable) {
    if (m_enabled != enable) {
        m_enabled = enable;
        QSettings settings;
        settings.setValue(u"notifications/enabled"_s, m_enabled);
        emit enabledChanged(m_enabled);
    }
}

uint LinuxNotificationService::showNotification(const QString& title, const QString& body, Urgency urgency,
                                                const QString& icon, int timeoutMs, const QStringList& actions,
                                                uint replacesId) {
    if (!m_enabled) {
        qCDebug(lcSpeech) << "LinuxNotificationService: Notifications disabled by user setting, skipping:" << title;
        return 0;
    }

    if (!QDBusConnection::sessionBus().isConnected()) {
        qCDebug(lcSpeech) << "LinuxNotificationService: D-Bus session bus not connected, skipping notification:"
                          << title;
        return 0;
    }

    QDBusInterface notifyIface(u"org.freedesktop.Notifications"_s, u"/org/freedesktop/Notifications"_s,
                               u"org.freedesktop.Notifications"_s, QDBusConnection::sessionBus());
    if (!notifyIface.isValid()) {
        qCDebug(lcSpeech) << "LinuxNotificationService: org.freedesktop.Notifications interface unavailable:"
                          << notifyIface.lastError().message();
        return 0;
    }

    QVariantMap hints;
    hints.insert(u"urgency"_s, QVariant::fromValue(static_cast<uchar>(urgency)));
    hints.insert(u"desktop-entry"_s, QStringLiteral("io.github.qtscribe"));

    const QDBusReply<uint> reply = notifyIface.call(u"Notify"_s, QStringLiteral("QtScribe"), replacesId, icon, title,
                                                    body, actions, hints, timeoutMs);

    if (!reply.isValid()) {
        qCWarning(lcSpeech) << "LinuxNotificationService: Failed to post notification:" << reply.error().message();
        return 0;
    }

    const uint notificationId = reply.value();
    qCInfo(lcSpeech) << "LinuxNotificationService: Notification posted successfully with ID:" << notificationId
                     << "Title:" << title << "Replaces ID:" << replacesId;
    return notificationId;
}

uint LinuxNotificationService::showCategoryNotification(const QString& categoryKey, const QString& title,
                                                        const QString& body, Urgency urgency, const QString& icon,
                                                        int timeoutMs, const QStringList& actions) {
    const uint replacesId = m_categoryIds.value(categoryKey, 0u);
    const uint newId = showNotification(title, body, urgency, icon, timeoutMs, actions, replacesId);
    if (newId > 0) {
        m_categoryIds.insert(categoryKey, newId);
    }
    return newId;
}

void LinuxNotificationService::clearCategory(const QString& categoryKey) {
    m_categoryIds.remove(categoryKey);
}

void LinuxNotificationService::onActionInvoked(uint id, const QString& actionKey) {
    qCDebug(lcSpeech) << "LinuxNotificationService: D-Bus ActionInvoked for ID:" << id << "Action:" << actionKey;
    emit actionInvoked(id, actionKey);
}

void LinuxNotificationService::onNotificationClosed(uint id, uint reason) {
    qCDebug(lcSpeech) << "LinuxNotificationService: D-Bus NotificationClosed for ID:" << id << "Reason:" << reason;
    for (auto it = m_categoryIds.begin(); it != m_categoryIds.end(); ++it) {
        if (it.value() == id) {
            m_categoryIds.erase(it);
            break;
        }
    }
    emit notificationClosed(id, reason);
}
