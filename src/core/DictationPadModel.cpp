#include "DictationPadModel.h"

#include <QClipboard>
#include <QGuiApplication>

using namespace Qt::StringLiterals;

DictationPadModel::DictationPadModel(QObject* parent)
    : QObject(parent) { }

QString DictationPadModel::text() const {
    return m_text;
}

void DictationPadModel::setText(const QString& text) {
    if (m_text != text) {
        m_text = text;
        emit textChanged();
    }
}

int DictationPadModel::wordCount() const {
    return calculateWordCount(m_text);
}

int DictationPadModel::charCount() const {
    return m_text.length();
}

void DictationPadModel::append(const QString& text) {
    if (text.isEmpty()) {
        return;
    }
    if (m_text.isEmpty()) {
        m_text = text;
    } else {
        m_text += u"\n"_s + text;
    }
    emit textChanged();
}

void DictationPadModel::clear() {
    if (!m_text.isEmpty()) {
        m_text.clear();
        emit textChanged();
    }
}

void DictationPadModel::copyToClipboard() {
    if (!m_text.isEmpty()) {
        copyTextToClipboard(m_text);
    }
}

void DictationPadModel::copyTextToClipboard(const QString& text) {
    if (QClipboard* clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
    }
}

int DictationPadModel::calculateWordCount(const QString& text) {
    int words = 0;
    bool inWord = false;
    for (const QChar ch : text) {
        if (ch.isSpace()) {
            inWord = false;
        } else if (!inWord) {
            inWord = true;
            ++words;
        }
    }
    return words;
}
