#pragma once

#include <QObject>
#include <QString>

class DictationCoordinator;

class DBusService : public QObject {
    Q_OBJECT

public:
    explicit DBusService(QObject* parent = nullptr);
    ~DBusService() override = default;

    bool registerService();
    bool registerController(DictationCoordinator* coordinator);
    bool isRegistered() const;

    static bool sendRemoteCommand(const QString& method);

    inline static constexpr QStringView kDbusServiceName = u"io.github.qtscribe.SpeechService";
    inline static constexpr QStringView kDbusObjectPath = u"/io/github/qtscribe/SpeechService";
    inline static constexpr QStringView kDbusInterfaceName = u"io.github.qtscribe.SpeechService";

private:
    bool m_registered = false;
};
