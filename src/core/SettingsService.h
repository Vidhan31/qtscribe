#pragma once

#include <QDBusVariant>
#include <QObject>
#include <QString>
#include <QVariantMap>

class SettingsManager;
class ApiKeyStore;

class SettingsService : public QObject {
    Q_OBJECT

public:
    explicit SettingsService(SettingsManager* manager, QObject* parent = nullptr);
    ~SettingsService() override = default;

    [[nodiscard]] SettingsManager* settingsManager() const noexcept { return m_manager; }
    void registerKeyStore(ApiKeyStore* store);

    void setSetting(const QString& key, const QDBusVariant& value);
    void setStringSetting(const QString& key, const QString& value);
    void setBoolSetting(const QString& key, bool value);
    void setIntSetting(const QString& key, int value);
    void setDoubleSetting(const QString& key, double value);
    [[nodiscard]] QDBusVariant getSetting(const QString& key) const;
    [[nodiscard]] QVariantMap allSettings() const;
    void reloadSettings();

signals:
    void settingChanged(const QString& key, const QDBusVariant& value);
    void settingsReloadRequested();

private:
    SettingsManager* m_manager = nullptr;
    QList<ApiKeyStore*> m_keyStores;
};
