#include "CloudSttRouter.h"

#include "LoggingCategories.h"

#include "CloudProviderModel.h"

using namespace Qt::StringLiterals;

CloudSttRouter::CloudSttRouter(QObject* parent)
    : AbstractSttClient(parent) { }

void CloudSttRouter::setCloudProviderModel(CloudProviderModel* model) {
    if (m_providerModel == model) {
        return;
    }

    if (m_providerModel) {
        disconnect(m_providerModel, &CloudProviderModel::activeProviderChanged, this,
                   &CloudSttRouter::onActiveProviderChanged);
    }

    m_providerModel = model;

    if (m_providerModel) {
        connect(m_providerModel, &CloudProviderModel::activeProviderChanged, this,
                &CloudSttRouter::onActiveProviderChanged);
    }

    updateActiveClient();
}

CloudProviderModel* CloudSttRouter::cloudProviderModel() const {
    return m_providerModel;
}

void CloudSttRouter::registerProvider(const QString& providerId, AbstractSttClient* client) {
    if (!client || providerId.isEmpty()) {
        return;
    }

    m_clients.insert(providerId, client);
    updateActiveClient();
}

void CloudSttRouter::unregisterProvider(const QString& providerId) {
    if (m_clients.remove(providerId) > 0) {
        updateActiveClient();
    }
}

AbstractSttClient* CloudSttRouter::activeCloudClient() const {
    return m_activeClient;
}

AbstractSttClient* CloudSttRouter::activeClient() const {
    return m_activeClient;
}

bool CloudSttRouter::isApiKeySet() const {
    return m_activeClient && m_activeClient->isApiKeySet();
}

bool CloudSttRouter::isApiKeyInvalid() const {
    return m_activeClient && m_activeClient->isApiKeyInvalid();
}

bool CloudSttRouter::isRateLimited() const {
    return m_activeClient && m_activeClient->isRateLimited();
}

int CloudSttRouter::retrySecondsRemaining() const {
    return m_activeClient ? m_activeClient->retrySecondsRemaining() : 0;
}

AbstractSttClient::ErrorCategory CloudSttRouter::errorCategory() const {
    return m_activeClient ? m_activeClient->errorCategory() : AbstractSttClient::ErrorCategory::None;
}

void CloudSttRouter::onActiveProviderChanged() {
    updateActiveClient();
}

void CloudSttRouter::updateActiveClient() {
    QString activeId = m_providerModel ? m_providerModel->activeProviderId() : u"groq"_s;
    auto* newActive = m_clients.value(activeId, nullptr);

    if (newActive == m_activeClient) {
        return;
    }

    if (m_activeClient) {
        disconnect(m_activeClient, &AbstractSttClient::transcriptionReady, this, &CloudSttRouter::transcriptionReady);
        disconnect(m_activeClient, &AbstractSttClient::errorOccurred, this, &CloudSttRouter::errorOccurred);
        disconnect(m_activeClient, &AbstractSttClient::busyChanged, this, &CloudSttRouter::busyChanged);
        disconnect(m_activeClient, &AbstractSttClient::readyChanged, this, &CloudSttRouter::readyChanged);
        disconnect(m_activeClient, &AbstractSttClient::apiKeySetChanged, this, &CloudSttRouter::apiKeySetChanged);
        disconnect(m_activeClient, &AbstractSttClient::isApiKeyInvalidChanged, this,
                   &CloudSttRouter::isApiKeyInvalidChanged);
        disconnect(m_activeClient, &AbstractSttClient::isRateLimitedChanged, this,
                   &CloudSttRouter::isRateLimitedChanged);
        disconnect(m_activeClient, &AbstractSttClient::retrySecondsRemainingChanged, this,
                   &CloudSttRouter::retrySecondsRemainingChanged);
        disconnect(m_activeClient, &AbstractSttClient::lastErrorChanged, this, &CloudSttRouter::lastErrorChanged);
        disconnect(m_activeClient, &AbstractSttClient::errorCategoryChanged, this,
                   &CloudSttRouter::errorCategoryChanged);
    }

    m_activeClient = newActive;

    if (m_activeClient) {
        connect(m_activeClient, &AbstractSttClient::transcriptionReady, this, &CloudSttRouter::transcriptionReady);
        connect(m_activeClient, &AbstractSttClient::errorOccurred, this, &CloudSttRouter::errorOccurred);
        connect(m_activeClient, &AbstractSttClient::busyChanged, this, &CloudSttRouter::busyChanged);
        connect(m_activeClient, &AbstractSttClient::readyChanged, this, &CloudSttRouter::readyChanged);
        connect(m_activeClient, &AbstractSttClient::apiKeySetChanged, this, &CloudSttRouter::apiKeySetChanged);
        connect(m_activeClient, &AbstractSttClient::isApiKeyInvalidChanged, this,
                &CloudSttRouter::isApiKeyInvalidChanged);
        connect(m_activeClient, &AbstractSttClient::isRateLimitedChanged, this, &CloudSttRouter::isRateLimitedChanged);
        connect(m_activeClient, &AbstractSttClient::retrySecondsRemainingChanged, this,
                &CloudSttRouter::retrySecondsRemainingChanged);
        connect(m_activeClient, &AbstractSttClient::lastErrorChanged, this, &CloudSttRouter::lastErrorChanged);
        connect(m_activeClient, &AbstractSttClient::errorCategoryChanged, this, &CloudSttRouter::errorCategoryChanged);
    }

    qCDebug(lcSpeech) << "CloudSttRouter: Active cloud provider switched to" << activeId
                      << "(client ready:" << (m_activeClient ? m_activeClient->isReady() : false) << ")";

    emit readyChanged();
    emit busyChanged();
    emit apiKeySetChanged();
    emit isApiKeyInvalidChanged();
    emit isRateLimitedChanged();
    emit retrySecondsRemainingChanged();
    emit lastErrorChanged();
    emit errorCategoryChanged();
}

void CloudSttRouter::transcribe(const QByteArray& wavData) {
    if (m_activeClient) {
        m_activeClient->transcribe(wavData);
    } else {
        emit errorOccurred(tr("No active cloud speech recognition provider configured."));
    }
}

void CloudSttRouter::cancel() {
    if (m_activeClient) {
        m_activeClient->cancel();
    }
}

void CloudSttRouter::retryLast() {
    if (m_activeClient) {
        m_activeClient->retryLast();
    }
}

bool CloudSttRouter::isReady() const {
    return m_activeClient && m_activeClient->isReady();
}

bool CloudSttRouter::isBusy() const {
    return m_activeClient && m_activeClient->isBusy();
}

bool CloudSttRouter::handlesSmartFormatting() const {
    return m_activeClient && m_activeClient->handlesSmartFormatting();
}

QString CloudSttRouter::lastError() const {
    return m_activeClient ? m_activeClient->lastError() : QString();
}

void CloudSttRouter::activate() {
    if (m_activeClient) {
        m_activeClient->activate();
    }
}

void CloudSttRouter::deactivate() {
    if (m_activeClient) {
        m_activeClient->deactivate();
    }
}
