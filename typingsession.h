#ifndef TYPINGSESSION_H
#define TYPINGSESSION_H

#pragma once

#include "keystrokeevent.h"

#include <QDateTime>
#include <QString>
#include <QUuid>
#include <QVector>

struct TypingSession
{
    QUuid id;
    QDateTime startedAtUtc;
    QVector<KeystrokeEvent> events;
    int ignoredAutoRepeatCount = 0;
    QString participantId;
    QString samplePurpose;
    QString textMode;
    QString promptLabel;

    int typedCharacterCount = 0;
    int promptCharacterCount = 0;
    int mistakeCount = 0;
    bool sampleValid = false;
};

#endif // TYPINGSESSION_H
