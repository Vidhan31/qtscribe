#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

class AbstractLlmClient : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)

public:
    explicit AbstractLlmClient(QObject* parent = nullptr);
    ~AbstractLlmClient() override = default;

    virtual bool isBusy() const = 0;
    virtual bool enabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual void processText(const QString& rawText) = 0;
    virtual void cancel() = 0;

signals:
    void busyChanged();
    void enabledChanged();
    void enhancementReady(const QString& enhancedText);
    void errorOccurred(const QString& error, const QString& fallbackRawText);
};
