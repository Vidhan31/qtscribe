#pragma once

#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class SettingsClient : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit SettingsClient(QObject* parent = nullptr);
    ~SettingsClient() override;

    static SettingsClient* instance();
    static SettingsClient* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    [[nodiscard]] bool isConnected() const noexcept;

signals:
    void connectedChanged();
    void settingChanged(const QString& key, const QVariant& value);

public slots:
    void setSetting(const QString& key, const QVariant& value);
    [[nodiscard]] QVariant getSetting(const QString& key, const QVariant& defaultValue = {}) const;
    void reloadSettings();

private:
    void setupInterface();
    Q_INVOKABLE void onServiceRegistered(const QString& serviceName);
    Q_INVOKABLE void onServiceUnregistered(const QString& serviceName);
    Q_INVOKABLE void onRemoteSettingChanged(const QString& key, const QDBusVariant& value);

    QDBusInterface* m_interface = nullptr;
    QDBusServiceWatcher* m_watcher = nullptr;
    bool m_connected = false;

    static inline SettingsClient* s_instance = nullptr;
};
