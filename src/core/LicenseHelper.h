#pragma once

#include <QFile>
#include <QObject>
#include <QQmlEngine>
#include <QString>

class LicenseHelper : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString licenseText READ licenseText CONSTANT FINAL)

public:
    explicit LicenseHelper(QObject* parent = nullptr)
        : QObject(parent) { }

    QString licenseText() const {
        if (!m_cachedLicenseText.isEmpty()) {
            return m_cachedLicenseText;
        }
        QFile file(QStringLiteral(":/qt/qml/QtScribe/license/LICENSE"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_cachedLicenseText = QString::fromUtf8(file.readAll());
            return m_cachedLicenseText;
        }
        m_cachedLicenseText = QStringLiteral("GNU GENERAL PUBLIC LICENSE\n"
                                             "Version 3, 29 June 2007\n\n"
                                             "Copyright (C) 2007 Free Software Foundation, Inc. <https://fsf.org/>\n"
                                             "Everyone is permitted to copy and distribute verbatim copies\n"
                                             "of this license document, but changing it is not allowed.\n");
        return m_cachedLicenseText;
    }

private:
    mutable QString m_cachedLicenseText;
};
