#ifndef SESSIONSUMMARY_H
#define SESSIONSUMMARY_H

#pragma once

#include <QtTypes>

struct SessionSummary
{
    int storedEvents = 0;
    int pressCount = 0;
    int releaseCount = 0;
    int ignoredAutoRepeatCount = 0;
    qint64 durationNs = 0;
    int dwellSampleCount = 0;
    int unmatchedReleaseCount = 0;
    int keysStillPressedCount = 0;
    double averageDwellMs = 0.0;
    double minDwellMs = 0.0;
    double maxDwellMs = 0.0;
    int overlapPressCount = 0;
    int flightSampleCount = 0;
    double averageFlightMs = 0.0;
    double minFlightMs = 0.0;
    double maxFlightMs = 0.0;
};

#endif // SESSIONSUMMARY_H
