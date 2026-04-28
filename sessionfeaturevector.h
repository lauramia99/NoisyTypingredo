#ifndef SESSIONFEATUREVECTOR_H
#define SESSIONFEATUREVECTOR_H

#pragma once

#include <QString>

struct SessionFeatureVector
{
    QString participantId;

    QString samplePurpose;
    QString textMode;
    QString promptLabel;

    QString sessionId;
    QString startedAtUtcIso;

    int storedEvents = 0;
    int pressCount = 0;
    int releaseCount = 0;
    int ignoredAutoRepeatCount = 0;
    int overlapPressCount = 0;
    int unmatchedReleaseCount = 0;
    int keysStillPressedCount = 0;

    double durationMs = 0.0;

    double averageDwellMs = 0.0;
    double minDwellMs = 0.0;
    double maxDwellMs = 0.0;

    double averageFlightMs = 0.0;
    double minFlightMs = 0.0;
    double maxFlightMs = 0.0;

    double overlapRatio = 0.0;
    double unmatchedReleaseRatio = 0.0;
    double ignoredRepeatRatio = 0.0;
};

#endif // SESSIONFEATUREVECTOR_H
