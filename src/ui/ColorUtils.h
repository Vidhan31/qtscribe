#pragma once

#include <QColor>
#include <QObject>
#include <QQmlEngine>
#include <QString>

#include <algorithm>

class ColorUtils : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged FINAL)
    Q_PROPERTY(bool isDark READ isDark NOTIFY isDarkChanged FINAL)
    Q_PROPERTY(bool systemThemeIsDark READ systemThemeIsDark NOTIFY systemThemeChanged FINAL)
    Q_PROPERTY(QString systemThemeName READ systemThemeName NOTIFY systemThemeChanged FINAL)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY systemThemeChanged FINAL)
    Q_PROPERTY(QColor accentColorHover READ accentColorHover NOTIFY systemThemeChanged FINAL)
    Q_PROPERTY(QColor accentColorPressed READ accentColorPressed NOTIFY systemThemeChanged FINAL)
    Q_PROPERTY(QColor focusRingColor READ focusRingColor NOTIFY systemThemeChanged FINAL)
    Q_PROPERTY(QColor sidebarItemSelected READ sidebarItemSelected NOTIFY systemThemeChanged FINAL)
    Q_PROPERTY(QColor selectedBg READ selectedBg NOTIFY systemThemeChanged FINAL)
    Q_PROPERTY(QColor buttonDangerBgHover READ buttonDangerBgHover NOTIFY isDarkChanged FINAL)

public:
    explicit ColorUtils(QObject* parent = nullptr);

    [[nodiscard]] QString themeMode() const;
    void setThemeMode(const QString& mode);

    [[nodiscard]] bool isDark() const;
    [[nodiscard]] bool systemThemeIsDark() const;
    [[nodiscard]] QString systemThemeName() const;
    [[nodiscard]] QColor accentColor() const;
    [[nodiscard]] QColor accentColorHover() const;
    [[nodiscard]] QColor accentColorPressed() const;
    [[nodiscard]] QColor focusRingColor() const;
    [[nodiscard]] QColor sidebarItemSelected() const;
    [[nodiscard]] QColor selectedBg() const;
    [[nodiscard]] QColor buttonDangerBgHover() const;

    Q_INVOKABLE static QColor withAlpha(const QColor& baseColor, qreal alpha);
    Q_INVOKABLE static QColor tint(const QColor& baseColor, const QColor& tintColor);
    Q_INVOKABLE QColor statusBg(const QColor& baseColor, qreal alpha = 0.12) const;
    Q_INVOKABLE QColor statusBorder(const QColor& baseColor, qreal alpha = 0.40) const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void themeModeChanged();
    void isDarkChanged();
    void systemThemeChanged();

private:
    void updateEffectiveTheme();

    QString m_themeMode;
    bool m_isDark {true};
    bool m_systemThemeIsDark {true};
};
