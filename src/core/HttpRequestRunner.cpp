#include "HttpRequestRunner.h"

bool RequestPolicy::isTransientError(const CloudApiResponse& res) const {
    if (res.isSuccess) {
        return false;
    }

    if (res.isRateLimited && res.retryAfterSeconds > 0 && res.retryAfterSeconds <= maxTransientRetryAfterSec) {
        return true;
    }

    if (res.httpStatus >= 500 && res.httpStatus <= 599) {
        return true;
    }

    switch (res.networkError) {
        case QNetworkReply::RemoteHostClosedError:
        case QNetworkReply::TemporaryNetworkFailureError:
        case QNetworkReply::TimeoutError:
        case QNetworkReply::NetworkSessionFailedError:
            return true;
        default:
            break;
    }

    return false;
}

bool RequestPolicy::shouldRetry(const CloudApiResponse& res, int currentRetryCount, bool cancelled) const {
    if (cancelled || currentRetryCount >= maxRetries) {
        return false;
    }
    return isTransientError(res);
}

int RequestPolicy::calculateRetryDelayMs(const CloudApiResponse& res) const {
    if (res.isRateLimited && res.retryAfterSeconds > 0) {
        return res.retryAfterSeconds * 1000;
    }
    return defaultDelayMs;
}

HttpRequestRunner::HttpRequestRunner(RequestPolicy policy)
    : m_policy(policy) { }

HttpRequestRunner::~HttpRequestRunner() {
    if (m_currentReply) {
        m_currentReply->abort();
    }
}

bool HttpRequestRunner::isBusy() const noexcept {
    return m_busy;
}

bool HttpRequestRunner::isCancelled() const noexcept {
    return m_cancelled;
}

int HttpRequestRunner::retryCount() const noexcept {
    return m_retryCount;
}

QPointer<QNetworkReply> HttpRequestRunner::currentReply() const noexcept {
    return m_currentReply;
}

const RequestPolicy& HttpRequestRunner::policy() const noexcept {
    return m_policy;
}

void HttpRequestRunner::setPolicy(const RequestPolicy& policy) noexcept {
    m_policy = policy;
}

void HttpRequestRunner::setBusy(bool busy) noexcept {
    m_busy = busy;
}

void HttpRequestRunner::setCurrentReply(QNetworkReply* reply) {
    m_currentReply = reply;
}

void HttpRequestRunner::prepareNewRequest() {
    m_cancelled = false;
    m_retryCount = 0;
    m_busy = true;
}

void HttpRequestRunner::reset() {
    m_retryCount = 0;
    m_currentReply = nullptr;
    m_busy = false;
}

void HttpRequestRunner::cancel() {
    m_cancelled = true;
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply = nullptr;
    }
    m_retryCount = 0;
    m_busy = false;
}

bool HttpRequestRunner::shouldRetry(const CloudApiResponse& res) const {
    return m_policy.shouldRetry(res, m_retryCount, m_cancelled);
}

int HttpRequestRunner::calculateRetryDelayMs(const CloudApiResponse& res) const {
    return m_policy.calculateRetryDelayMs(res);
}

void HttpRequestRunner::incrementRetryCount() noexcept {
    ++m_retryCount;
}
