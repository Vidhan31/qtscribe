#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringView>

class ApiKeyStore : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged FINAL)
    Q_PROPERTY(bool apiKeySet READ apiKeySet NOTIFY apiKeySetChanged FINAL)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged FINAL)

public:
    explicit ApiKeyStore(QObject* parent = nullptr);
    ~ApiKeyStore() override = default;

    QString apiKey() const;
    void setApiKey(const QString& key);
    bool apiKeySet() const;
    bool isLoading() const;

    Q_INVOKABLE void loadApiKey();
    void ensureApiKeyLoaded();

    void setStorageKeys(const QString& keychainService, const QString& keychainKey,
                        const QString& settingsKey = QString());

    QString keychainService() const;
    QString keychainKey() const;
    QString settingsKey() const;

signals:
    void apiKeyChanged();
    void apiKeySetChanged();
    void isLoadingChanged();

private:
    void setIsLoading(bool loading);
    void loadApiKeyFromKeychain();
    void loadApiKeyFromSettingsFallback();
    void saveApiKeyToKeychain(const QString& key);
    void deleteApiKeyFromKeychain();

    QString m_apiKey;
    bool m_apiKeyLoaded = false;
    bool m_isLoadingApiKey = false;

    inline static constexpr QStringView kKeychainService = u"QtScribe";
    inline static constexpr QStringView kKeychainKey = u"groq_api_key";
    inline static constexpr QStringView kSettingsApiKey = u"Cloud/Providers/Groq/ApiKey";
    inline static constexpr QStringView kLegacySettingsApiKey = u"Groq/ApiKey";

    QString m_keychainService = kKeychainService.toString();
    QString m_keychainKey = kKeychainKey.toString();
    QString m_settingsKey = kSettingsApiKey.toString();
};
