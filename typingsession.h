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
};


#endif // TYPINGSESSION_H
