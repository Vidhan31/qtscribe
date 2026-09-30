#pragma once

#include <QObject>
#include <QString>

class AbstractTextInjector : public QObject {
    Q_OBJECT

public:
    explicit AbstractTextInjector(QObject* parent = nullptr);
    ~AbstractTextInjector() override = default;

    virtual bool inject(const QString& text) = 0;
    virtual void cancel() = 0;
};
