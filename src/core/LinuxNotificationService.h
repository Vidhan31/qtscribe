#pragma once

#include <QHash>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>

#include <cstdint>

class LinuxNotificationService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class Urgency { Low = 0, Normal = 1, Critical = 2 };
    Q_ENUM(Urgency)

    explicit LinuxNotificationService(QObject* parent = nullptr);
    ~LinuxNotificationService() override = default;

    bool isEnabled() const;
    void setEnabled(bool enable);

    Q_INVOKABLE virtual uint showNotification(const QString& title, const QString& body,
                                              Urgency urgency = Urgency::Normal,
                                              const QString& icon = QStringLiteral("dialog-information"),
                                              int timeoutMs = 5000, const QStringList& actions = {},
                                              uint replacesId = 0);

    Q_INVOKABLE virtual uint showCategoryNotification(const QString& categoryKey, const QString& title,
                                                      const QString& body, Urgency urgency = Urgency::Normal,
                                                      const QString& icon = QStringLiteral("dialog-information"),
                                                      int timeoutMs = 5000, const QStringList& actions = {});

    Q_INVOKABLE virtual void clearCategory(const QString& categoryKey);

signals:
    void enabledChanged(bool enabled);
    void actionInvoked(uint id, const QString& actionKey);
    void notificationClosed(uint id, uint reason);

public slots:
    void onActionInvoked(uint id, const QString& actionKey);
    void onNotificationClosed(uint id, uint reason);

private:
    bool m_enabled = true;
    QHash<QString, uint> m_categoryIds;
};
