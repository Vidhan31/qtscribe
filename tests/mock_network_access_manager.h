#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <cstring>

class MockNetworkReply : public QNetworkReply {
public:
    MockNetworkReply(int statusCode, const QByteArray& data,
                     const QList<QPair<QByteArray, QByteArray>>& headers = {},
                     NetworkError netErr = NoError, const QString& errorStr = QString(),
                     QObject* parent = nullptr)
        : QNetworkReply(parent), m_data(data), m_offset(0) {
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, statusCode);
        for (const auto& [hdr, val] : headers) {
            setRawHeader(hdr, val);
        }
        if (netErr != NoError) {
            setError(netErr, errorStr);
        }
        open(ReadOnly | Unbuffered);
        setFinished(true);
        QTimer::singleShot(0, this, [this]() {
            emit readyRead();
            emit finished();
        });
    }

    void abort() override {
        close();
        setError(OperationCanceledError, QStringLiteral("Operation canceled"));
        setFinished(true);
        emit finished();
    }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override {
        return (m_data.size() - m_offset) + QIODevice::bytesAvailable();
    }

    qint64 readData(char* data, qint64 maxlen) override {
        if (m_offset >= m_data.size()) {
            return 0;
        }
        const qint64 bytesToRead = qMin(maxlen, static_cast<qint64>(m_data.size() - m_offset));
        std::memcpy(data, m_data.constData() + m_offset, static_cast<size_t>(bytesToRead));
        m_offset += bytesToRead;
        return bytesToRead;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QByteArray m_data;
    qint64 m_offset;
};

class MockNetworkAccessManager : public QNetworkAccessManager {
public:
    explicit MockNetworkAccessManager(QObject* parent = nullptr) : QNetworkAccessManager(parent) {}

    int nextStatusCode = 200;
    QByteArray nextResponseData;
    QList<QPair<QByteArray, QByteArray>> nextHeaders;
    QNetworkReply::NetworkError nextError = QNetworkReply::NoError;
    QString nextErrorString;

    QNetworkRequest lastRequest;
    QByteArray lastOutgoingData;
    Operation lastOperation = GetOperation;
    int requestCount = 0;

protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& req, QIODevice* outgoingData) override {
        lastOperation = op;
        lastRequest = req;
        lastOutgoingData = outgoingData ? outgoingData->readAll() : QByteArray();
        requestCount++;

        return new MockNetworkReply(nextStatusCode, nextResponseData, nextHeaders, nextError, nextErrorString, this);
    }
};
