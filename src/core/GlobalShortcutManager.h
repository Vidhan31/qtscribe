#pragma once

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantMap>

struct PortalShortcut {
    QString id;
    QVariantMap options;
};
Q_DECLARE_METATYPE(PortalShortcut)

using ShortcutList = QList<PortalShortcut>;
Q_DECLARE_METATYPE(ShortcutList)

QDBusArgument& operator<<(QDBusArgument& argument, const PortalShortcut& shortcut);
const QDBusArgument& operator>>(const QDBusArgument& argument, PortalShortcut& shortcut);

class GlobalShortcutManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged FINAL)
    Q_PROPERTY(bool supported READ isSupported NOTIFY supportedChanged FINAL)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged FINAL)

public:
    explicit GlobalShortcutManager(QObject* parent = nullptr);
    ~GlobalShortcutManager() override = default;

    bool isAvailable() const;
    bool isSupported() const;
    QString statusMessage() const;

    Q_INVOKABLE void requestShortcuts();

signals:
    void shortcutActivated(const QString& shortcutId);
    void shortcutDeactivated(const QString& shortcutId);
    void availableChanged();
    void supportedChanged();
    void statusMessageChanged();

public slots:
    void initializePortal();

private slots:
    void onCreateSessionResponse(uint responseCode, const QVariantMap& results);
    void onBindShortcutsResponse(uint responseCode, const QVariantMap& results);
    void onShortcutActivated(const QDBusObjectPath& sessionHandle, const QString& shortcutId, qulonglong timestamp,
                             const QVariantMap& options);
    void onShortcutDeactivated(const QDBusObjectPath& sessionHandle, const QString& shortcutId, qulonglong timestamp,
                               const QVariantMap& options);

protected:
    void setAvailable(bool available);
    void setSupported(bool supported);
    void setStatusMessage(const QString& msg);

private:
    void registerHostApp();
    void createSession();
    void bindShortcuts();

    QDBusObjectPath m_sessionHandle;
    bool m_available = false;
    bool m_supported = true;
    bool m_sessionRequested = false;
    QString m_statusMessage;
    QString m_handleToken;
    QString m_sessionHandleToken;
};
