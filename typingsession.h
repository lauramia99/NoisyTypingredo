#ifndef TYPINGSESSION_H
#define TYPINGSESSION_H

#pragma once

#include "keystrokeevent.h"

#include <QDateTime>
#include <QUuid>
#include <QVector>

struct TypingSession
{
    QUuid id;
    QDateTime startedAtUtc;
    QVector<KeystrokeEvent> events;
    int ignoredAutoRepeatCount = 0;

};


#endif // TYPINGSESSION_H
