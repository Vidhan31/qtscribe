#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class AbstractSttClient : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged FINAL)
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)

public:
    explicit AbstractSttClient(QObject* parent = nullptr);
    ~AbstractSttClient() override = default;

    virtual void transcribe(const QByteArray& wavData) = 0;
    virtual void cancel() = 0;
    virtual void retryLast();
    virtual bool isReady() const = 0;
    virtual bool isBusy() const = 0;

    virtual void activate();
    virtual void deactivate();

    virtual QString lastError() const;
    virtual bool handlesSmartFormatting() const;

signals:
    void transcriptionReady(const QString& text);
    void errorOccurred(const QString& error);
    void busyChanged();
    void readyChanged();
    void lastErrorChanged();
};
