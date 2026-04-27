#include "typingtextedit.h"

#include <QKeyEvent>

typingtextedit::typingtextedit(QWidget *parent)
    : QPlainTextEdit(parent)
{
    timer_.start();
}

void typingtextedit::keyPressEvent(QKeyEvent *event)
{
    emit keyPressed(event->key(), timer_.nsecsElapsed(), event->isAutoRepeat());
    QPlainTextEdit::keyPressEvent(event);
}

void typingtextedit::keyReleaseEvent(QKeyEvent *event)
{
    emit keyReleased(event->key(), timer_.nsecsElapsed(), event->isAutoRepeat());
    QPlainTextEdit::keyReleaseEvent(event);
}



