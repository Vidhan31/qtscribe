#include "CloudProviderModel.h"

#include <QSettings>

using namespace Qt::StringLiterals;

CloudProviderModel::CloudProviderModel(QObject* parent)
    : QAbstractListModel(parent) {
    initializeProviders();
    loadSettings();
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

    endResetModel();
    emit countChanged();
}

void CloudProviderModel::loadSettings() {
    QSettings settings;
    const auto savedProvider = settings.value(u"Cloud/ActiveProvider"_s, u"groq"_s).toString();
    if (indexOfProvider(savedProvider) >= 0) {
        m_activeProviderId = savedProvider;
    } else {
        m_activeProviderId = u"groq"_s;
    }
}

void CloudProviderModel::saveSettings() {
    QSettings settings;
    settings.setValue(u"Cloud/ActiveProvider"_s, m_activeProviderId);
}
