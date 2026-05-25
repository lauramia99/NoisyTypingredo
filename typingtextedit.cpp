#include "typingtextedit.h"

#include <QColor>
#include <QKeyEvent>
#include <QMimeData>
#include <QSignalBlocker>
#include <QTextCharFormat>
#include <QTextCursor>

static KeystrokeEvent buildKeystrokeEvent(KeyAction action, QKeyEvent *event, qint64 timestampNs)
{
    return KeystrokeEvent{
        action,
        event->key(),
        event->nativeScanCode(),
        event->nativeVirtualKey(),
        event->nativeModifiers(),
        timestampNs,
        event->isAutoRepeat()
    };
}

typingtextedit::typingtextedit(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setReadOnly(true);
    setUndoRedoEnabled(false);
    setStyleSheet(
        "QPlainTextEdit {"
        "background-color: #2b2b2b;"
        "color: #9b9b9b;"
        "selection-background-color: #3a3a3a;"
        "selection-color: #9b9b9b;"
        "}");
    timer_.start();
}

void typingtextedit::setPromptText(const QString &promptText)
{
    if (promptText_ == promptText)
    {
        return;
    }

    promptText_ = promptText;
    typedText_.clear();
    setPlainText(promptText_);
    timer_.restart();
    refreshPromptDisplay();
    emit typingProgressChanged();
}

QString typingtextedit::promptText() const
{
    return promptText_;
}

QString typingtextedit::typedText() const
{
    return typedText_;
}

int typingtextedit::typedCharacterCount() const
{
    return typedText_.size();
}

int typingtextedit::promptCharacterCount() const
{
    return promptText_.size();
}

int typingtextedit::mistakeCount() const
{
    if (promptText_.isEmpty())
    {
        return 0;
    }

    int mistakes = 0;

    const int comparableLength = qMin(typedText_.size(), promptText_.size());

    for (int i = 0; i < comparableLength; ++i)
    {
        if (typedText_.at(i) != promptText_.at(i))
        {
            ++mistakes;
        }
    }

    if (typedText_.size() > promptText_.size())
    {
        mistakes += typedText_.size() - promptText_.size();
    }

    return mistakes;
}

bool typingtextedit::isCompleteAndCorrect() const
{
    return !promptText_.isEmpty() && typedText_ == promptText_;
}

void typingtextedit::resetTypedText()
{
    typedText_.clear();
    timer_.restart();
    refreshPromptDisplay();
    emit typingProgressChanged();
}

void typingtextedit::keyPressEvent(QKeyEvent *event)
{
    const KeystrokeEvent capturedEvent =
        buildKeystrokeEvent(KeyAction::Press, event, timer_.nsecsElapsed());

    emit keystrokeCaptured(capturedEvent);

    if (event->isAutoRepeat())
    {
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Backspace)
    {
        if (!typedText_.isEmpty())
        {
            typedText_.chop(1);
            refreshPromptDisplay();
            emit typingProgressChanged();
        }

        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Enter ||
        event->key() == Qt::Key_Tab)
    {
        event->accept();
        return;
    }

    const bool isShortcut =
        event->modifiers().testFlag(Qt::ControlModifier) ||
        event->modifiers().testFlag(Qt::AltModifier) ||
        event->modifiers().testFlag(Qt::MetaModifier);

    if (!isShortcut && !event->text().isEmpty())
    {
        typedText_.append(event->text());
        refreshPromptDisplay();
        emit typingProgressChanged();
        event->accept();
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void typingtextedit::keyReleaseEvent(QKeyEvent *event)
{
    const KeystrokeEvent capturedEvent =
        buildKeystrokeEvent(KeyAction::Release, event, timer_.nsecsElapsed());

    emit keystrokeCaptured(capturedEvent);
    QPlainTextEdit::keyReleaseEvent(event);
}

void typingtextedit::insertFromMimeData(const QMimeData *source)
{
    Q_UNUSED(source);
}

void typingtextedit::refreshPromptDisplay()
{
    const QSignalBlocker blocker(this);

    setUpdatesEnabled(false);

    const bool hasPrompt = !promptText_.isEmpty();

    const QString visibleText =
        hasPrompt
            ? (typedText_.size() > promptText_.size()
                ? promptText_ + typedText_.mid(promptText_.size())
                : promptText_)
            : typedText_;

    if (document()->toPlainText() != visibleText)
    {
        setPlainText(visibleText);
    }

    QTextCursor cursor(document());
    cursor.beginEditBlock();

    QTextCharFormat baseFormat;
    baseFormat.setForeground(QColor(155, 155, 155));
    baseFormat.setBackground(QColor(43, 43, 43));
    baseFormat.setUnderlineStyle(QTextCharFormat::NoUnderline);

    cursor.select(QTextCursor::Document);
    cursor.mergeCharFormat(baseFormat);

    for (int i = 0; i < visibleText.size(); ++i)
    {
        QTextCharFormat format;
        format.setForeground(QColor(155, 155, 155));
        format.setBackground(QColor(43, 43, 43));
        format.setUnderlineStyle(QTextCharFormat::NoUnderline);

        if (!hasPrompt)
        {
            format.setForeground(QColor(245, 245, 245));
        }

        else if (i < typedText_.size() && i < promptText_.size())
        {
            const bool isCorrect = typedText_.at(i) == promptText_.at(i);

            if (isCorrect)
            {
                format.setForeground(QColor(245, 245, 245));
            }
            else
            {
                format.setForeground(QColor(210, 95, 95));
                format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
                format.setUnderlineColor(QColor(210, 95, 95));
            }
        }
        else if (i >= promptText_.size())
        {
            format.setForeground(QColor(210, 95, 95));
            format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
            format.setUnderlineColor(QColor(210, 95, 95));
        }
        else if (i == typedText_.size())
        {
            format.setBackground(QColor(58, 58, 58));
        }

        cursor.setPosition(i);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        cursor.mergeCharFormat(format);
    }

    cursor.endEditBlock();
    setExtraSelections({});

    cursor.setPosition(hasPrompt
                       ? qMin(typedText_.size(), promptText_.size())
                       : typedText_.size());

    setTextCursor(cursor);

    setUpdatesEnabled(true);
    viewport()->update();
}

