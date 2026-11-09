#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>

class DictationCoordinator;
class SystemService;
class StatusNotifierService;

struct KDbusImageStruct {
    int width = 0;
    int height = 0;
    QByteArray data;
};
Q_DECLARE_METATYPE(KDbusImageStruct)

using KDbusImageVector = QList<KDbusImageStruct>;
Q_DECLARE_METATYPE(KDbusImageVector)

struct KDbusToolTipStruct {
    QString icon;
    KDbusImageVector image;
    QString title;
    QString subTitle;
};
Q_DECLARE_METATYPE(KDbusToolTipStruct)

struct DBusMenuLayoutItem {
    int id = 0;
    QVariantMap properties;
    QList<DBusMenuLayoutItem> children;
};
Q_DECLARE_METATYPE(DBusMenuLayoutItem)

struct DBusMenuItem {
    int id = 0;
    QVariantMap properties;
};
Q_DECLARE_METATYPE(DBusMenuItem)
using DBusMenuItemList = QList<DBusMenuItem>;
Q_DECLARE_METATYPE(DBusMenuItemList)

struct DBusMenuItemKeys {
    int id = 0;
    QStringList keys;
};
Q_DECLARE_METATYPE(DBusMenuItemKeys)
using DBusMenuItemKeysList = QList<DBusMenuItemKeys>;
Q_DECLARE_METATYPE(DBusMenuItemKeysList)

QDBusArgument& operator<<(QDBusArgument& argument, const KDbusImageStruct& icon);
const QDBusArgument& operator>>(const QDBusArgument& argument, KDbusImageStruct& icon);

QDBusArgument& operator<<(QDBusArgument& argument, const KDbusToolTipStruct& toolTip);
const QDBusArgument& operator>>(const QDBusArgument& argument, KDbusToolTipStruct& toolTip);

QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuLayoutItem& item);
const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuLayoutItem& item);

QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuItem& item);
const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuItem& item);

QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuItemKeys& item);
const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuItemKeys& item);

class StatusNotifierItemAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierItem")
    Q_PROPERTY(QString Category READ category)
    Q_PROPERTY(QString Id READ id)
    Q_PROPERTY(QString Title READ title)
    Q_PROPERTY(QString Status READ status)
    Q_PROPERTY(int WindowId READ windowId)
    Q_PROPERTY(QString IconThemePath READ iconThemePath)
    Q_PROPERTY(QDBusObjectPath Menu READ menu)
    Q_PROPERTY(bool ItemIsMenu READ itemIsMenu)
    Q_PROPERTY(QString IconName READ iconName)
    Q_PROPERTY(KDbusImageVector IconPixmap READ iconPixmap)
    Q_PROPERTY(QString OverlayIconName READ overlayIconName)
    Q_PROPERTY(KDbusImageVector OverlayIconPixmap READ overlayIconPixmap)
    Q_PROPERTY(QString AttentionIconName READ attentionIconName)
    Q_PROPERTY(KDbusImageVector AttentionIconPixmap READ attentionIconPixmap)
    Q_PROPERTY(QString AttentionMovieName READ attentionMovieName)
    Q_PROPERTY(KDbusToolTipStruct ToolTip READ toolTip)

public:
    explicit StatusNotifierItemAdaptor(StatusNotifierService* parent);

    static QString category();
    static QString id();
    static QString title();
    static QString status();
    static int windowId();
    static QString iconThemePath();
    static QDBusObjectPath menu();
    static bool itemIsMenu();
    static QString iconName();
    static KDbusImageVector iconPixmap();
    static QString overlayIconName();
    static KDbusImageVector overlayIconPixmap();
    static QString attentionIconName();
    static KDbusImageVector attentionIconPixmap();
    static QString attentionMovieName();
    KDbusToolTipStruct toolTip() const;

Q_SIGNALS:
    void NewTitle();
    void NewIcon();
    void NewAttentionIcon();
    void NewOverlayIcon();
    void NewToolTip();
    void NewStatus(const QString& status);
    void NewMenu();

public Q_SLOTS:
    static void ContextMenu(int x, int y);
    void Activate(int x, int y);
    static void SecondaryActivate(int x, int y);
    static void Scroll(int delta, const QString& orientation);
    static void ProvideXdgActivationToken(const QString& token);

private:
    StatusNotifierService* m_service = nullptr;
};

class DBusMenuAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.dbusmenu")
    Q_PROPERTY(uint Version READ version)
    Q_PROPERTY(QString Status READ status)

public:
    explicit DBusMenuAdaptor(StatusNotifierService* parent);

    static uint version() { return 3; }
    static QString status();

Q_SIGNALS:
    void ItemsPropertiesUpdated(const DBusMenuItemList& updatedProps, const DBusMenuItemKeysList& removedProps);
    void LayoutUpdated(uint revision, int parent);
    void ItemActivationRequested(int id, uint timestamp);

public Q_SLOTS:
    static bool AboutToShow(int id);
    void Event(int id, const QString& eventId, const QDBusVariant& data, uint timestamp);
    QList<int> EventGroup(const QList<int>& ids, const QString& eventId, const QDBusVariant& data, uint timestamp);
    static bool AboutToShowGroup(const QList<int>& ids, QList<int>& updatesNeeded, QList<int>& idErrors);
    static QDBusVariant GetProperty(int id, const QString& name);
    uint GetLayout(int parentId, int recursionDepth, const QStringList& propertyNames, DBusMenuLayoutItem& layout);
    static DBusMenuItemList GetGroupProperties(const QList<int>& ids, const QStringList& propertyNames);

private:
    StatusNotifierService* m_service = nullptr;
};

class StatusNotifierService : public QObject {
    Q_OBJECT

public:
    explicit StatusNotifierService(QObject* parent = nullptr);
    ~StatusNotifierService() override;

    bool registerController(DictationCoordinator* coordinator, SystemService* systemService = nullptr);
    DictationCoordinator* controller() const { return m_coordinator; }
    SystemService* systemService() const { return m_systemService; }

    QString serviceName() const { return m_serviceName; }
    bool isRegistered() const { return m_registered; }
    uint revision() const { return m_revision; }

    Q_INVOKABLE void registerWithWatcher();

signals:
    void requestShowWindow();
    void requestQuitApp();

private:
    void onRecordingChanged();

    static void registerMetaTypes();

    DictationCoordinator* m_coordinator = nullptr;
    SystemService* m_systemService = nullptr;
    StatusNotifierItemAdaptor* m_sniAdaptor = nullptr;
    DBusMenuAdaptor* m_menuAdaptor = nullptr;
    QString m_serviceName;
    bool m_registered = false;
    uint m_revision = 1;
};
