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
class StatusNotifierService;

struct KDbusImageStruct {
    int width = 0;
    int height = 0;
    QByteArray data;
};
Q_DECLARE_METATYPE(KDbusImageStruct)

typedef QList<KDbusImageStruct> KDbusImageVector;
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
typedef QList<DBusMenuItem> DBusMenuItemList;
Q_DECLARE_METATYPE(DBusMenuItemList)

struct DBusMenuItemKeys {
    int id = 0;
    QStringList keys;
};
Q_DECLARE_METATYPE(DBusMenuItemKeys)
typedef QList<DBusMenuItemKeys> DBusMenuItemKeysList;
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

    QString category() const;
    QString id() const;
    QString title() const;
    QString status() const;
    int windowId() const;
    QString iconThemePath() const;
    QDBusObjectPath menu() const;
    bool itemIsMenu() const;
    QString iconName() const;
    KDbusImageVector iconPixmap() const;
    QString overlayIconName() const;
    KDbusImageVector overlayIconPixmap() const;
    QString attentionIconName() const;
    KDbusImageVector attentionIconPixmap() const;
    QString attentionMovieName() const;
    KDbusToolTipStruct toolTip() const;

public Q_SLOTS:
    void ContextMenu(int x, int y);
    void Activate(int x, int y);
    void SecondaryActivate(int x, int y);
    void Scroll(int delta, const QString& orientation);
    void ProvideXdgActivationToken(const QString& token);

Q_SIGNALS:
    void NewTitle();
    void NewIcon();
    void NewAttentionIcon();
    void NewOverlayIcon();
    void NewToolTip();
    void NewStatus(const QString& status);
    void NewMenu();

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

    uint version() const { return 3; }
    QString status() const;

public Q_SLOTS:
    bool AboutToShow(int id);
    void Event(int id, const QString& eventId, const QDBusVariant& data, uint timestamp);
    QList<int> EventGroup(const QList<int>& ids, const QString& eventId, const QDBusVariant& data, uint timestamp);
    bool AboutToShowGroup(const QList<int>& ids, QList<int>& updatesNeeded, QList<int>& idErrors);
    QDBusVariant GetProperty(int id, const QString& name);
    uint GetLayout(int parentId, int recursionDepth, const QStringList& propertyNames, DBusMenuLayoutItem& layout);
    DBusMenuItemList GetGroupProperties(const QList<int>& ids, const QStringList& propertyNames);

Q_SIGNALS:
    void ItemsPropertiesUpdated(const DBusMenuItemList& updatedProps, const DBusMenuItemKeysList& removedProps);
    void LayoutUpdated(uint revision, int parent);
    void ItemActivationRequested(int id, uint timestamp);

private:
    StatusNotifierService* m_service = nullptr;
};

class StatusNotifierService : public QObject {
    Q_OBJECT

public:
    explicit StatusNotifierService(QObject* parent = nullptr);
    ~StatusNotifierService() override;

    bool registerController(DictationCoordinator* coordinator);
    DictationCoordinator* controller() const { return m_coordinator; }

    QString serviceName() const { return m_serviceName; }
    bool isRegistered() const { return m_registered; }
    uint revision() const { return m_revision; }

public Q_SLOTS:
    void registerWithWatcher();

private Q_SLOTS:
    void onRecordingChanged();

private:
    static void registerMetaTypes();

    DictationCoordinator* m_coordinator = nullptr;
    StatusNotifierItemAdaptor* m_sniAdaptor = nullptr;
    DBusMenuAdaptor* m_menuAdaptor = nullptr;
    QString m_serviceName;
    bool m_registered = false;
    uint m_revision = 1;
};
