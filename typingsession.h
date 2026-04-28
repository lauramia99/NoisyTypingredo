#ifndef TYPINGSESSION_H
#define TYPINGSESSION_H

#pragma once

#include "keystrokeevent.h"

#include <QDateTime>
#include <QUuid>
#include <QVector>
#include <QString>

struct TypingSession
{
    QUuid id;
    QDateTime startedAtUtc;
    QVector<KeystrokeEvent> events;
    int ignoredAutoRepeatCount = 0;
    QString participantId;


};


#endif // TYPINGSESSION_H
