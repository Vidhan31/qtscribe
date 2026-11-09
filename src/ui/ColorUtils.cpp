#include "ColorUtils.h"

#include "common/SettingsKeys.h"

#include <QEvent>
#include <QGuiApplication>
#include <QPalette>
#include <QSettings>
#include <QStyleHints>

#include <cmath>

using namespace Qt::StringLiterals;

ColorUtils::ColorUtils(QObject* parent)
    : QObject(parent) {
    const QSettings settings;
    m_themeMode = settings.value(SettingsKeys::kUiThemeMode.toString(), u"system"_s).toString();

    if (auto* app = qGuiApp) {
        if (auto* hints = QGuiApplication::styleHints()) {
            connect(hints, &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme) { updateEffectiveTheme(); });
        }
        app->installEventFilter(this);
    }

    updateEffectiveTheme();
}

bool ColorUtils::eventFilter(QObject* watched, QEvent* event) {
    if (event && event->type() == QEvent::ApplicationPaletteChange) {
        updateEffectiveTheme();
    }
    return QObject::eventFilter(watched, event);
}

QString ColorUtils::themeMode() const {
    return m_themeMode;
}

void ColorUtils::setThemeMode(const QString& mode) {
    if (m_themeMode == mode) {
        return;
    }

    m_themeMode = mode;
    QSettings settings;
    settings.setValue(SettingsKeys::kUiThemeMode.toString(), m_themeMode);

    emit themeModeChanged();
    updateEffectiveTheme();
}

bool ColorUtils::isDark() const {
    return m_isDark;
}

bool ColorUtils::systemThemeIsDark() const {
    return m_systemThemeIsDark;
}

QString ColorUtils::systemThemeName() const {
    return m_systemThemeIsDark ? u"Dark"_s : u"Light"_s;
}

QColor ColorUtils::accentColor() const {
    const auto palette = QGuiApplication::palette();

    auto isValidAccent = [](const QColor& c) -> bool {
        if (!c.isValid() || c.alphaF() < 0.5F) {
            return false;
        }
        const float sum = c.redF() + c.greenF() + c.blueF();
        if (sum < 0.2F || (c.redF() > 0.95F && c.greenF() > 0.95F && c.blueF() > 0.95F)) {
            return false;
        }
        if (c.lightnessF() < 0.15F || c.lightnessF() > 0.92F) {
            return false;
        }
        if (c.hslSaturationF() < 0.15F && std::abs(c.redF() - c.greenF()) < 0.05F &&
            std::abs(c.greenF() - c.blueF()) < 0.05F) {
            return false;
        }
        return true;
    };

    QColor accent = palette.color(QPalette::Accent);
    if (isValidAccent(accent)) {
        return accent;
    }

    QColor highlight = palette.color(QPalette::Highlight);
    if (isValidAccent(highlight)) {
        return highlight;
    }

    return m_isDark ? QColor(u"#3584E4"_s) : QColor(u"#1C71D8"_s);
}

QColor ColorUtils::accentColorHover() const {
    return tint(accentColor(), m_isDark ? QColor(255, 255, 255, 38) : QColor(0, 0, 0, 26));
}

QColor ColorUtils::accentColorPressed() const {
    return tint(accentColor(), QColor(0, 0, 0, 51));
}

QColor ColorUtils::focusRingColor() const {
    return withAlpha(accentColor(), 0.6);
}

QColor ColorUtils::sidebarItemSelected() const {
    return withAlpha(accentColor(), m_isDark ? 0.22 : 0.16);
}

QColor ColorUtils::selectedBg() const {
    return withAlpha(accentColor(), m_isDark ? 0.18 : 0.14);
}

QColor ColorUtils::buttonDangerBgHover() const {
    const QColor danger = m_isDark ? QColor(u"#FF453A"_s) : QColor(u"#FF3B30"_s);
    return tint(danger, m_isDark ? QColor(255, 255, 255, 26) : QColor(0, 0, 0, 20));
}

void ColorUtils::updateEffectiveTheme() {
    const auto* hints = QGuiApplication::styleHints();
    const auto scheme = hints ? hints->colorScheme() : Qt::ColorScheme::Unknown;
    const bool sysDark =
        (scheme == Qt::ColorScheme::Dark) ||
        (scheme != Qt::ColorScheme::Light && QGuiApplication::palette().color(QPalette::Window).lightnessF() < 0.5);

    if (m_systemThemeIsDark != sysDark) {
        m_systemThemeIsDark = sysDark;
        emit systemThemeChanged();
    }

    const bool effectiveDark = [&]() {
        if (m_themeMode == u"dark"_s) {
            return true;
        }
        if (m_themeMode == u"light"_s) {
            return false;
        }
        return sysDark;
    }();

    if (m_isDark != effectiveDark) {
        m_isDark = effectiveDark;
        emit isDarkChanged();
    }
}

QColor ColorUtils::withAlpha(const QColor& baseColor, qreal alpha) {
    QColor c = baseColor;
    c.setAlphaF(static_cast<float>(std::clamp(alpha, 0.0, 1.0)));
    return c;
}

QColor ColorUtils::tint(const QColor& baseColor, const QColor& tintColor) {
    const float a = tintColor.alphaF();
    return QColor::fromRgbF(std::lerp(baseColor.redF(), tintColor.redF(), a),
                            std::lerp(baseColor.greenF(), tintColor.greenF(), a),
                            std::lerp(baseColor.blueF(), tintColor.blueF(), a), baseColor.alphaF());
}

QColor ColorUtils::statusBg(const QColor& baseColor, qreal alpha) const {
    return withAlpha(baseColor, m_isDark ? alpha : alpha * 0.85);
}

QColor ColorUtils::statusBorder(const QColor& baseColor, qreal alpha) const {
    return withAlpha(baseColor, m_isDark ? alpha : alpha * 0.75);
}
