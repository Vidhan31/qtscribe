#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

class DictationPadModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(int wordCount READ wordCount NOTIFY textChanged FINAL)
    Q_PROPERTY(int charCount READ charCount NOTIFY textChanged FINAL)

public:
    explicit DictationPadModel(QObject* parent = nullptr);
    ~DictationPadModel() override = default;

    QString text() const;
    void setText(const QString& text);

    int wordCount() const;
    int charCount() const;

    static int calculateWordCount(const QString& text);
    Q_INVOKABLE static void copyTextToClipboard(const QString& text);

public slots:
    void append(const QString& text);
    void clear();
    void copyToClipboard();

signals:
    void textChanged();

private:
    QString m_text;
};
