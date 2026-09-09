#include "AbstractSttClient.h"

AbstractSttClient::AbstractSttClient(QObject* parent)
    : QObject(parent) { }

void AbstractSttClient::activate() { }

void AbstractSttClient::deactivate() { }

void AbstractSttClient::retryLast() { }

QString AbstractSttClient::lastError() const {
    return {};
}

bool AbstractSttClient::handlesSmartFormatting() const {
    return false;
}

bool AbstractSttClient::isApiKeySet() const {
    return true;
}

bool AbstractSttClient::isApiKeyInvalid() const {
    return false;
}

bool AbstractSttClient::isRateLimited() const {
    return false;
}

int AbstractSttClient::retrySecondsRemaining() const {
    return 0;
}

AbstractSttClient::ErrorCategory AbstractSttClient::errorCategory() const {
    return ErrorCategory::None;
}
