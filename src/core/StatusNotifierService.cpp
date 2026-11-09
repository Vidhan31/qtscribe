#include "StatusNotifierService.h"

#include "DictationCoordinator.h"
#include "LoggingCategories.h"

#include "SystemService.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusServiceWatcher>

using namespace Qt::StringLiterals;

QDBusArgument& operator<<(QDBusArgument& argument, const KDbusImageStruct& icon) {
    argument.beginStructure();
    argument << icon.width << icon.height << icon.data;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, KDbusImageStruct& icon) {
    argument.beginStructure();
    argument >> icon.width >> icon.height >> icon.data;
    argument.endStructure();
    return argument;
}

QDBusArgument& operator<<(QDBusArgument& argument, const KDbusToolTipStruct& toolTip) {
    argument.beginStructure();
    argument << toolTip.icon << toolTip.image << toolTip.title << toolTip.subTitle;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, KDbusToolTipStruct& toolTip) {
    argument.beginStructure();
    argument >> toolTip.icon >> toolTip.image >> toolTip.title >> toolTip.subTitle;
    argument.endStructure();
    return argument;
}

QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuLayoutItem& item) {
    argument.beginStructure();
    argument << item.id << item.properties;
    argument.beginArray(qMetaTypeId<QDBusVariant>());
    for (const auto& child : item.children) {
        QDBusVariant var;
        var.setVariant(QVariant::fromValue(child));
        argument << var;
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuLayoutItem& item) {
    argument.beginStructure();
    argument >> item.id >> item.properties;
    argument.beginArray();
    while (!argument.atEnd()) {
        QDBusVariant var;
        argument >> var;
        item.children.append(qvariant_cast<DBusMenuLayoutItem>(var.variant()));
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuItem& item) {
    argument.beginStructure();
    argument << item.id << item.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuItem& item) {
    argument.beginStructure();
    argument >> item.id >> item.properties;
    argument.endStructure();
    return argument;
}

QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuItemKeys& item) {
    argument.beginStructure();
    argument << item.id << item.keys;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuItemKeys& item) {
    argument.beginStructure();
    argument >> item.id >> item.keys;
    argument.endStructure();
    return argument;
}

StatusNotifierItemAdaptor::StatusNotifierItemAdaptor(StatusNotifierService* parent)
    : QDBusAbstractAdaptor(parent)
    , m_service(parent) { }

QString StatusNotifierItemAdaptor::category() {
    return u"ApplicationStatus"_s;
}

QString StatusNotifierItemAdaptor::id() {
    return u"qtscribe"_s;
}

QString StatusNotifierItemAdaptor::title() {
    return u"QtScribe"_s;
}

QString StatusNotifierItemAdaptor::status() {
    return u"Active"_s;
}

int StatusNotifierItemAdaptor::windowId() {
    return 0;
}

QString StatusNotifierItemAdaptor::iconThemePath() {
    return {};
}

QDBusObjectPath StatusNotifierItemAdaptor::menu() {
    return QDBusObjectPath(u"/MenuBar"_s);
}

bool StatusNotifierItemAdaptor::itemIsMenu() {
    return false;
}

QString StatusNotifierItemAdaptor::iconName() {
    return u"io.github.qtscribe"_s;
}

KDbusImageVector StatusNotifierItemAdaptor::iconPixmap() {
    return {};
}

QString StatusNotifierItemAdaptor::overlayIconName() {
    return {};
}

KDbusImageVector StatusNotifierItemAdaptor::overlayIconPixmap() {
    return {};
}

QString StatusNotifierItemAdaptor::attentionIconName() {
    return {};
}

KDbusImageVector StatusNotifierItemAdaptor::attentionIconPixmap() {
    return {};
}

QString StatusNotifierItemAdaptor::attentionMovieName() {
    return {};
}

KDbusToolTipStruct StatusNotifierItemAdaptor::toolTip() const {
    KDbusToolTipStruct tip;
    const bool rec = m_service->controller() && m_service->controller()->recording();
    tip.icon = u"io.github.qtscribe"_s;
    tip.title = u"QtScribe"_s;
    tip.subTitle = rec ? u"Recording..."_s : u"Ready"_s;
    return tip;
}

void StatusNotifierItemAdaptor::ContextMenu(int x, int y) {
    Q_UNUSED(x);
    Q_UNUSED(y);
}

void StatusNotifierItemAdaptor::Activate(int x, int y) {
    Q_UNUSED(x);
    Q_UNUSED(y);
    emit m_service->requestShowWindow();
}

void StatusNotifierItemAdaptor::SecondaryActivate(int x, int y) {
    Q_UNUSED(x);
    Q_UNUSED(y);
}

void StatusNotifierItemAdaptor::Scroll(int delta, const QString& orientation) {
    Q_UNUSED(delta);
    Q_UNUSED(orientation);
}

void StatusNotifierItemAdaptor::ProvideXdgActivationToken(const QString& token) {
    if (!token.isEmpty()) {
        qputenv("XDG_ACTIVATION_TOKEN", token.toUtf8());
    }
}

DBusMenuAdaptor::DBusMenuAdaptor(StatusNotifierService* parent)
    : QDBusAbstractAdaptor(parent)
    , m_service(parent) { }

QString DBusMenuAdaptor::status() {
    return u"normal"_s;
}

bool DBusMenuAdaptor::AboutToShow(int id) {
    Q_UNUSED(id);
    return false;
}

void DBusMenuAdaptor::Event(int id, const QString& eventId, const QDBusVariant& data, uint timestamp) {
    Q_UNUSED(data);
    Q_UNUSED(timestamp);
    if (eventId == u"clicked"_s) {
        if (id == 1) {
            emit m_service->requestShowWindow();
        } else if (id == 3) {
            emit m_service->requestQuitApp();
        }
    }
}

QList<int> DBusMenuAdaptor::EventGroup(const QList<int>& ids, const QString& eventId, const QDBusVariant& data,
                                       uint timestamp) {
    for (const int id : ids) {
        Event(id, eventId, data, timestamp);
    }
    return {};
}

bool DBusMenuAdaptor::AboutToShowGroup(const QList<int>& ids, QList<int>& updatesNeeded, QList<int>& idErrors) {
    Q_UNUSED(ids);
    updatesNeeded.clear();
    idErrors.clear();
    return true;
}

QDBusVariant DBusMenuAdaptor::GetProperty(int id, const QString& name) {
    if (id == 1) {
        if (name == u"label"_s) {
            return QDBusVariant(u"Open QtScribe"_s);
        }
        if (name == u"enabled"_s) {
            return QDBusVariant(true);
        }
        if (name == u"visible"_s) {
            return QDBusVariant(true);
        }
    } else if (id == 2) {
        if (name == u"type"_s) {
            return QDBusVariant(u"separator"_s);
        }
        if (name == u"visible"_s) {
            return QDBusVariant(true);
        }
    } else if (id == 3) {
        if (name == u"label"_s) {
            return QDBusVariant(u"Quit"_s);
        }
        if (name == u"enabled"_s) {
            return QDBusVariant(true);
        }
        if (name == u"visible"_s) {
            return QDBusVariant(true);
        }
    }
    return {};
}

uint DBusMenuAdaptor::GetLayout(int parentId, int recursionDepth, const QStringList& propertyNames,
                                DBusMenuLayoutItem& layout) {
    Q_UNUSED(recursionDepth);
    Q_UNUSED(propertyNames);

    layout.id = parentId;
    layout.properties.clear();
    layout.children.clear();

    if (parentId != 0) {
        return m_service->revision();
    }

    DBusMenuLayoutItem showItem;
    showItem.id = 1;
    showItem.properties[u"label"_s] = u"Open QtScribe"_s;
    showItem.properties[u"enabled"_s] = true;
    showItem.properties[u"visible"_s] = true;

    DBusMenuLayoutItem sepItem;
    sepItem.id = 2;
    sepItem.properties[u"type"_s] = u"separator"_s;
    sepItem.properties[u"visible"_s] = true;

    DBusMenuLayoutItem quitItem;
    quitItem.id = 3;
    quitItem.properties[u"label"_s] = u"Quit"_s;
    quitItem.properties[u"enabled"_s] = true;
    quitItem.properties[u"visible"_s] = true;

    layout.children.append(showItem);
    layout.children.append(sepItem);
    layout.children.append(quitItem);

    return m_service->revision();
}

DBusMenuItemList DBusMenuAdaptor::GetGroupProperties(const QList<int>& ids, const QStringList& propertyNames) {
    Q_UNUSED(propertyNames);
    DBusMenuItemList list;

    for (const int id : ids) {
        DBusMenuItem item;
        item.id = id;
        if (id == 1) {
            item.properties[u"label"_s] = u"Open QtScribe"_s;
            item.properties[u"enabled"_s] = true;
            item.properties[u"visible"_s] = true;
        } else if (id == 2) {
            item.properties[u"type"_s] = u"separator"_s;
            item.properties[u"visible"_s] = true;
        } else if (id == 3) {
            item.properties[u"label"_s] = u"Quit"_s;
            item.properties[u"enabled"_s] = true;
            item.properties[u"visible"_s] = true;
        }
        list.append(item);
    }
    return list;
}

void StatusNotifierService::registerMetaTypes() {
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;

    qDBusRegisterMetaType<KDbusImageStruct>();
    qDBusRegisterMetaType<KDbusImageVector>();
    qDBusRegisterMetaType<KDbusToolTipStruct>();
    qDBusRegisterMetaType<DBusMenuLayoutItem>();
    qDBusRegisterMetaType<DBusMenuItem>();
    qDBusRegisterMetaType<DBusMenuItemList>();
    qDBusRegisterMetaType<DBusMenuItemKeys>();
    qDBusRegisterMetaType<DBusMenuItemKeysList>();
}

StatusNotifierService::StatusNotifierService(QObject* parent)
    : QObject(parent) {
    registerMetaTypes();
    m_sniAdaptor = new StatusNotifierItemAdaptor(this);
    m_menuAdaptor = new DBusMenuAdaptor(this);
}

StatusNotifierService::~StatusNotifierService() {
    if (m_registered && QDBusConnection::sessionBus().isConnected()) {
        QDBusConnection::sessionBus().unregisterObject(u"/StatusNotifierItem"_s);
        QDBusConnection::sessionBus().unregisterObject(u"/MenuBar"_s);
        QDBusConnection::sessionBus().unregisterService(m_serviceName);
    }
}

bool StatusNotifierService::registerController(DictationCoordinator* coordinator, SystemService* systemService) {
    if (!coordinator || !QDBusConnection::sessionBus().isConnected()) {
        return false;
    }

    m_coordinator = coordinator;
    m_systemService = systemService;
    connect(m_coordinator, &DictationCoordinator::recordingChanged, this, &StatusNotifierService::onRecordingChanged);

    m_serviceName = QString(u"org.kde.StatusNotifierItem-%1-1"_s).arg(QCoreApplication::applicationPid());

    if (!QDBusConnection::sessionBus().registerService(m_serviceName)) {
        qWarning() << "StatusNotifierService: Failed to register D-Bus service" << m_serviceName;
        return false;
    }

    if (!QDBusConnection::sessionBus().registerObject(u"/StatusNotifierItem"_s, this)) {
        qWarning() << "StatusNotifierService: Failed to register /StatusNotifierItem";
        return false;
    }

    if (!QDBusConnection::sessionBus().registerObject(u"/MenuBar"_s, this)) {
        qWarning() << "StatusNotifierService: Failed to register /MenuBar";
        return false;
    }

    m_registered = true;
    qCDebug(lcSpeech) << "StatusNotifierService: Registered D-Bus SNI service" << m_serviceName;

    registerWithWatcher();

    auto* serviceWatcher = new QDBusServiceWatcher(u"org.kde.StatusNotifierWatcher"_s, QDBusConnection::sessionBus(),
                                                   QDBusServiceWatcher::WatchForRegistration, this);
    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &StatusNotifierService::registerWithWatcher);

    QDBusConnection::sessionBus().connect(u"org.kde.StatusNotifierWatcher"_s, u"/StatusNotifierWatcher"_s,
                                          u"org.kde.StatusNotifierWatcher"_s, u"StatusNotifierWatcherRegistered"_s,
                                          this, SLOT(registerWithWatcher()));

    return true;
}

void StatusNotifierService::registerWithWatcher() {
    if (!QDBusConnection::sessionBus().isConnected() || m_serviceName.isEmpty()) {
        return;
    }

    QDBusInterface watcher(u"org.kde.StatusNotifierWatcher"_s, u"/StatusNotifierWatcher"_s,
                           u"org.kde.StatusNotifierWatcher"_s, QDBusConnection::sessionBus());

    if (watcher.isValid()) {
        const QDBusMessage reply = watcher.call(u"RegisterStatusNotifierItem"_s, m_serviceName);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qCDebug(lcSpeech) << "StatusNotifierService: RegisterStatusNotifierItem returned:" << reply.errorMessage();
        } else {
            qCDebug(lcSpeech) << "StatusNotifierService: Registered with StatusNotifierWatcher";
        }
    }
}

void StatusNotifierService::onRecordingChanged() {
    m_revision++;
    if (m_sniAdaptor) {
        emit m_sniAdaptor->NewIcon();
        emit m_sniAdaptor->NewToolTip();
    }
    if (m_menuAdaptor) {
        emit m_menuAdaptor->LayoutUpdated(m_revision, 0);
    }
}
