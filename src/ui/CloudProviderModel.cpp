#include "CloudProviderModel.h"

#include "common/SettingsKeys.h"
#include "DaemonClient.h"
#include "SettingsClient.h"

#include <QDebug>
#include <QSettings>

using namespace Qt::StringLiterals;

CloudProviderModel::CloudProviderModel(QObject* parent)
    : QAbstractListModel(parent) {
    initializeProviders();
    loadSettings();

    if (auto* sc = SettingsClient::instance()) {
        connect(sc, &SettingsClient::settingChanged, this, [this](const QString& key, const QVariant& value) {
            if (key == SettingsKeys::kCloudActiveProvider) {
                const QString id = value.toString();
                if (m_activeProviderId != id) {
                    setActiveProviderId(id);
                }
            }
        });
    }
}

int CloudProviderModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_providers.size());
}

QVariant CloudProviderModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_providers.size())) {
        return {};
    }

    const auto& item = m_providers.at(index.row());
    switch (role) {
        case IdRole:
            return item.id;
        case NameRole:
            return item.name;
        case DescriptionRole:
            return item.description;
        case IconSourceRole:
            return item.iconSource;
        case ComponentSourceRole:
            return item.componentSource;
        case WebsiteUrlRole:
            return item.websiteUrl;
        case IsActiveRole:
            return item.id == m_activeProviderId;
        default:
            return {};
    }
}

QHash<int, QByteArray> CloudProviderModel::roleNames() const {
    return {{IdRole, "providerId"},
            {NameRole, "name"},
            {DescriptionRole, "description"},
            {IconSourceRole, "iconSource"},
            {ComponentSourceRole, "componentSource"},
            {WebsiteUrlRole, "websiteUrl"},
            {IsActiveRole, "isActive"}};
}

QString CloudProviderModel::activeProviderId() const {
    return m_activeProviderId;
}

void CloudProviderModel::setActiveProviderId(const QString& id) {
    if (m_activeProviderId == id) {
        return;
    }

    if (indexOfProvider(id) < 0) {
        return;
    }

    m_activeProviderId = id;
    saveSettings();

    emit dataChanged(index(0, 0), index(rowCount() - 1, 0), {IsActiveRole});
    emit activeProviderChanged();
}

int CloudProviderModel::activeProviderIndex() const {
    return indexOfProvider(m_activeProviderId);
}

void CloudProviderModel::setActiveProviderIndex(int index) {
    if (index >= 0 && index < static_cast<int>(m_providers.size())) {
        setActiveProviderId(m_providers.at(index).id);
    }
}

QString CloudProviderModel::activeProviderName() const {
    const int idx = activeProviderIndex();
    if (idx >= 0 && idx < static_cast<int>(m_providers.size())) {
        return m_providers.at(idx).name;
    }
    return tr("Cloud Provider");
}

QString CloudProviderModel::activeProviderDescription() const {
    const int idx = activeProviderIndex();
    if (idx >= 0 && idx < static_cast<int>(m_providers.size())) {
        return m_providers.at(idx).description;
    }
    return {};
}

QString CloudProviderModel::activeProviderComponent() const {
    const int idx = activeProviderIndex();
    if (idx >= 0 && idx < static_cast<int>(m_providers.size())) {
        return m_providers.at(idx).componentSource;
    }
    return u"providers/GroqProviderSettings.qml"_s;
}

QString CloudProviderModel::activeProviderWebsiteUrl() const {
    const int idx = activeProviderIndex();
    if (idx >= 0 && idx < static_cast<int>(m_providers.size())) {
        return m_providers.at(idx).websiteUrl;
    }
    return {};
}

int CloudProviderModel::count() const {
    return static_cast<int>(m_providers.size());
}

int CloudProviderModel::indexOfProvider(const QString& id) const {
    for (int i = 0; i < static_cast<int>(m_providers.size()); ++i) {
        if (m_providers.at(i).id == id) {
            return i;
        }
    }
    return -1;
}

QString CloudProviderModel::providerIdAt(int index) const {
    if (index >= 0 && index < static_cast<int>(m_providers.size())) {
        return m_providers.at(index).id;
    }
    return {};
}

void CloudProviderModel::initializeProviders() {
    beginResetModel();
    m_providers.clear();

    m_providers.append({
        .id = u"groq"_s,
        .name = tr("Groq Cloud"),
        .description =
            tr("Ultra-fast cloud Whisper speech recognition and LLM text enhancement powered by Groq LPU compute."),
        .iconSource = u"qrc:/qt/qml/QtScribe/assets/icons/sparkles.svg"_s,
        .componentSource = u"providers/GroqProviderSettings.qml"_s,
        .websiteUrl = u"https://console.groq.com/keys"_s,
        .isDefault = true,
    });

    m_providers.append({
        .id = u"gemini"_s,
        .name = tr("Google Gemini"),
        .description =
            tr("Next-generation speech recognition with Gemini 3.5 Transcribe featuring built-in smart formatting."),
        .iconSource = u"qrc:/qt/qml/QtScribe/assets/icons/sparkles.svg"_s,
        .componentSource = u"providers/GeminiProviderSettings.qml"_s,
        .websiteUrl = u"https://aistudio.google.com/app/apikey"_s,
        .isDefault = false,
    });

    m_providers.append({
        .id = u"meta-musa"_s,
        .name = tr("Meta Musa"),
        .description = tr("High-accuracy multilingual speech transcription powered by Meta Muse Voice Transcribe."),
        .iconSource = u"qrc:/qt/qml/QtScribe/assets/icons/sparkles.svg"_s,
        .componentSource = u"providers/MetaMusaProviderSettings.qml"_s,
        .websiteUrl = u"https://dev.meta.ai/"_s,
        .isDefault = false,
    });

    endResetModel();
    emit countChanged();
}

void CloudProviderModel::loadSettings() {
    QString savedProvider = u"groq"_s;
    if (auto* sc = SettingsClient::instance()) {
        savedProvider = sc->getSetting(SettingsKeys::kCloudActiveProvider.toString(), u"groq"_s).toString();
    }
    const QString targetId = (indexOfProvider(savedProvider) >= 0) ? savedProvider : u"groq"_s;
    if (m_activeProviderId != targetId) {
        m_activeProviderId = targetId;
        if (rowCount() > 0) {
            emit dataChanged(index(0, 0), index(rowCount() - 1, 0), {IsActiveRole});
        }
        emit activeProviderChanged();
    }
}

void CloudProviderModel::saveSettings() {
    if (auto* sc = SettingsClient::instance()) {
        sc->setSetting(SettingsKeys::kCloudActiveProvider.toString(), m_activeProviderId);
    }
}
