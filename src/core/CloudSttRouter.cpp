#include "CloudSttRouter.h"

#include "GroqSttClient.h"
#include "LoggingCategories.h"

#include "GeminiSttClient.h"

#include <QSettings>

using namespace Qt::StringLiterals;

static inline CloudSttRouter* s_cloudRouterInstance = nullptr;

CloudSttRouter::CloudSttRouter(QObject* parent)
    : AbstractSttClient(parent) {
    if (!s_cloudRouterInstance) {
        s_cloudRouterInstance = this;
    }
    loadSettings();
    initDefaultProviders();
}

CloudSttRouter::~CloudSttRouter() {
    if (s_cloudRouterInstance == this) {
        s_cloudRouterInstance = nullptr;
    }
}

CloudSttRouter* CloudSttRouter::instance() {
    return s_cloudRouterInstance;
}

CloudSttRouter* CloudSttRouter::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
    if (!s_cloudRouterInstance) {
        s_cloudRouterInstance = new CloudSttRouter();
    }
    return s_cloudRouterInstance;
}

void CloudSttRouter::initDefaultProviders() {
    if (!m_clients.contains(u"groq"_s)) {
        auto* groq = GroqSttClient::instance();
        registerProvider(u"groq"_s, groq ? groq : new GroqSttClient(this));
    }
    if (!m_clients.contains(u"gemini"_s)) {
        auto* gemini = GeminiSttClient::instance();
        registerProvider(u"gemini"_s, gemini ? gemini : new GeminiSttClient(this));
    }
}

void CloudSttRouter::loadSettings() {
    QSettings settings;
    setActiveProviderId(settings.value(u"Cloud/ActiveProvider"_s, u"groq"_s).toString());
}

QString CloudSttRouter::activeProviderId() const {
    return m_activeProviderId;
}

void CloudSttRouter::setActiveProviderId(const QString& providerId) {
    if (m_activeProviderId == providerId && m_activeClient) {
        return;
    }
    m_activeProviderId = providerId;
    updateActiveClient();
    emit activeProviderChanged(m_activeProviderId);
}

void CloudSttRouter::registerProvider(const QString& providerId, AbstractSttClient* client) {
    if (!client || providerId.isEmpty()) {
        return;
    }

    auto* oldClient = m_clients.value(providerId, nullptr);
    if (oldClient && oldClient != client && oldClient->parent() == this) {
        oldClient->deleteLater();
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

void CloudSttRouter::updateActiveClient() {
    auto* newActive = m_clients.value(m_activeProviderId, nullptr);

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

    qCDebug(lcSpeech) << "CloudSttRouter: Active cloud provider switched to" << m_activeProviderId
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
