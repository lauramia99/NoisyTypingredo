#include "typingtextedit.h"

#include <QKeyEvent>

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
    timer_.start();
}

void typingtextedit::keyPressEvent(QKeyEvent *event)
{
    const KeystrokeEvent capturedEvent =
        buildKeystrokeEvent(KeyAction::Press, event, timer_.nsecsElapsed());

    emit keystrokeCaptured(capturedEvent);
    QPlainTextEdit::keyPressEvent(event);
}

void typingtextedit::keyReleaseEvent(QKeyEvent *event)
{
    const KeystrokeEvent capturedEvent =
        buildKeystrokeEvent(KeyAction::Release, event, timer_.nsecsElapsed());

    emit keystrokeCaptured(capturedEvent);
    QPlainTextEdit::keyReleaseEvent(event);
}

