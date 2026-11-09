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
