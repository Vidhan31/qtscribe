#pragma once

#include "CloudResponseParser.h"

#include <QNetworkReply>
#include <QPointer>

struct RequestPolicy {
    int maxRetries = 1;
    int defaultDelayMs = 1000;
    int maxTransientRetryAfterSec = 3;

    [[nodiscard]] bool shouldRetry(const CloudApiResponse& res, int currentRetryCount, bool cancelled) const;
    [[nodiscard]] int calculateRetryDelayMs(const CloudApiResponse& res) const;
    [[nodiscard]] bool isTransientError(const CloudApiResponse& res) const;
};

class HttpRequestRunner {
public:
    explicit HttpRequestRunner(RequestPolicy policy = {});
    ~HttpRequestRunner();

    HttpRequestRunner(const HttpRequestRunner&) = delete;
    HttpRequestRunner& operator=(const HttpRequestRunner&) = delete;
    HttpRequestRunner(HttpRequestRunner&&) noexcept = default;
    HttpRequestRunner& operator=(HttpRequestRunner&&) noexcept = default;

    [[nodiscard]] bool isBusy() const noexcept;
    [[nodiscard]] bool isCancelled() const noexcept;
    [[nodiscard]] int retryCount() const noexcept;
    [[nodiscard]] QPointer<QNetworkReply> currentReply() const noexcept;
    [[nodiscard]] const RequestPolicy& policy() const noexcept;

    void setPolicy(const RequestPolicy& policy) noexcept;
    void setBusy(bool busy) noexcept;
    void setCurrentReply(QNetworkReply* reply);

    void prepareNewRequest();
    void reset();
    void cancel();

    [[nodiscard]] bool shouldRetry(const CloudApiResponse& res) const;
    [[nodiscard]] int calculateRetryDelayMs(const CloudApiResponse& res) const;
    void incrementRetryCount() noexcept;

private:
    RequestPolicy m_policy;
    QPointer<QNetworkReply> m_currentReply;
    int m_retryCount = 0;
    bool m_busy = false;
    bool m_cancelled = false;
};
