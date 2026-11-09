#include "AbstractCloudSttClient.h"

AbstractCloudSttClient::AbstractCloudSttClient(QObject* parent)
    : AbstractSttClient(parent) { }

bool AbstractCloudSttClient::isApiKeySet() const {
    return false;
}

bool AbstractCloudSttClient::isApiKeyInvalid() const {
    return false;
}

bool AbstractCloudSttClient::isRateLimited() const {
    return false;
}

int AbstractCloudSttClient::retrySecondsRemaining() const {
    return 0;
}

AbstractCloudSttClient::ErrorCategory AbstractCloudSttClient::errorCategory() const {
    return ErrorCategory::None;
}
