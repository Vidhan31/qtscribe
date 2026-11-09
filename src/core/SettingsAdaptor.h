#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusVariant>
#include <QString>
#include <QVariantMap>

class SettingsService;

class SettingsAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.qtscribe.SettingsService")

public:
    explicit SettingsAdaptor(SettingsService* service, QObject* parent);
    ~SettingsAdaptor() override = default;

signals:
    void settingChanged(const QString& key, const QDBusVariant& value);

public slots:
    void setSetting(const QString& key, const QDBusVariant& value);
    void setStringSetting(const QString& key, const QString& value);
    void setBoolSetting(const QString& key, bool value);
    void setIntSetting(const QString& key, int value);
    void setDoubleSetting(const QString& key, double value);
    [[nodiscard]] QDBusVariant getSetting(const QString& key) const;
    [[nodiscard]] QVariantMap allSettings() const;
    void reloadSettings();

private:
    SettingsService* m_service = nullptr;
};
