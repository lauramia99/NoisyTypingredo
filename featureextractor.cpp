#include "featureextractor.h"

#include <QHash>

namespace
{
quint64 makePhysicalKeyId(const KeystrokeEvent &event)
{
    if (event.nativeScanCode != 0)
    {
        return (static_cast<quint64>(1) << 32) | event.nativeScanCode;
    }

    return (static_cast<quint64>(2) << 32) | static_cast<quint32>(event.key);
}

void fillDwellStats(const TypingSession &session, SessionSummary &summary)
{
    QHash<quint64, QVector<qint64>> pressedKeys;
    qint64 totalDwellNs = 0;
    bool hasDwellSample = false;
    qint64 minDwellNs = 0;
    qint64 maxDwellNs = 0;

    for (const KeystrokeEvent &event : session.events)
    {
        const quint64 keyId = makePhysicalKeyId(event);

        if (event.action == KeyAction::Press)
        {
            pressedKeys[keyId].append(event.timestampNs);
            continue;
        }

        auto it = pressedKeys.find(keyId);
        if (it == pressedKeys.end() || it->isEmpty())
        {
            ++summary.unmatchedReleaseCount;
            continue;
        }

        const qint64 pressTimestampNs = it->first();
        it->removeFirst();

        if (it->isEmpty())
        {
            pressedKeys.erase(it);
        }

        const qint64 dwellNs = event.timestampNs - pressTimestampNs;

        if (dwellNs < 0)
        {
            ++summary.unmatchedReleaseCount;
            continue;
        }

        totalDwellNs += dwellNs;
        ++summary.dwellSampleCount;

        if (!hasDwellSample)
        {
            minDwellNs = dwellNs;
            maxDwellNs = dwellNs;
            hasDwellSample = true;
        }
        else
        {
            if (dwellNs < minDwellNs)
            {
                minDwellNs = dwellNs;
            }

            if (dwellNs > maxDwellNs)
            {
                maxDwellNs = dwellNs;
            }
        }
    }

    summary.keysStillPressedCount = 0;

    for (auto it = pressedKeys.cbegin(); it != pressedKeys.cend(); ++it)
    {
        summary.keysStillPressedCount += it->size();
    }

    if (summary.dwellSampleCount > 0)
    {
        summary.averageDwellMs =
            static_cast<double>(totalDwellNs) / summary.dwellSampleCount / 1000000.0;
        summary.minDwellMs = static_cast<double>(minDwellNs) / 1000000.0;
        summary.maxDwellMs = static_cast<double>(maxDwellNs) / 1000000.0;
    }
}

void fillFlightStats(const TypingSession &session, SessionSummary &summary)
{
    bool hasPreviousPress = false;
    qint64 previousPressTimestampNs = 0;
    qint64 totalFlightNs = 0;
    bool hasFlightSample = false;
    qint64 minFlightNs = 0;
    qint64 maxFlightNs = 0;
    QHash<quint64, int> activePressCounts;
    int activePressedKeyCount = 0;

    for (const KeystrokeEvent &event : session.events)
    {
        const quint64 keyId = makePhysicalKeyId(event);

        if (event.action == KeyAction::Press)
        {
            if (activePressedKeyCount > 0)
            {
                ++summary.overlapPressCount;
            }

            if (hasPreviousPress)
            {
                const qint64 flightNs = event.timestampNs - previousPressTimestampNs;
                totalFlightNs += flightNs;
                ++summary.flightSampleCount;

                if (!hasFlightSample)
                {
                    minFlightNs = flightNs;
                    maxFlightNs = flightNs;
                    hasFlightSample = true;
                }
                else
                {
                    if (flightNs < minFlightNs)
                    {
                        minFlightNs = flightNs;
                    }

                    if (flightNs > maxFlightNs)
                    {
                        maxFlightNs = flightNs;
                    }
                }
            }

            previousPressTimestampNs = event.timestampNs;
            hasPreviousPress = true;

            activePressCounts[keyId] += 1;
            activePressedKeyCount += 1;
        }
        else
        {
            auto it = activePressCounts.find(keyId);

            if (it != activePressCounts.end() && it.value() > 0)
            {
                it.value() -= 1;
                activePressedKeyCount -= 1;

                if (it.value() == 0)
                {
                    activePressCounts.erase(it);
                }
            }
        }
    }

    if (summary.flightSampleCount > 0)
    {
        summary.averageFlightMs =
            static_cast<double>(totalFlightNs) / summary.flightSampleCount / 1000000.0;
        summary.minFlightMs = static_cast<double>(minFlightNs) / 1000000.0;
        summary.maxFlightMs = static_cast<double>(maxFlightNs) / 1000000.0;
    }
}
}

SessionSummary FeatureExtractor::buildSessionSummary(const TypingSession &session)
{
    SessionSummary summary;
    summary.storedEvents = session.events.size();
    summary.ignoredAutoRepeatCount = session.ignoredAutoRepeatCount;

    for (const KeystrokeEvent &event : session.events)
    {
        if (event.action == KeyAction::Press)
        {
            ++summary.pressCount;
        }
        else
        {
            ++summary.releaseCount;
        }
    }

    if (!session.events.isEmpty())
    {
        summary.durationNs =
            session.events.last().timestampNs - session.events.first().timestampNs;
    }

    fillDwellStats(session, summary);
    fillFlightStats(session, summary);
    return summary;
}

SessionFeatureVector FeatureExtractor::buildFeatureVector(const TypingSession &session,
                                                          const SessionSummary &summary)
{
    SessionFeatureVector vector;

    vector.participantId = session.participantId;
    vector.samplePurpose = session.samplePurpose;
    vector.textMode = session.textMode;
    vector.promptLabel = session.promptLabel;

    vector.sessionId = session.id.toString(QUuid::WithoutBraces);
    vector.startedAtUtcIso = session.startedAtUtc.toString(Qt::ISODateWithMs);

    vector.typedCharacterCount = session.typedCharacterCount;
    vector.promptCharacterCount = session.promptCharacterCount;
    vector.mistakeCount = session.mistakeCount;
    vector.sampleValid = session.sampleValid;

    vector.storedEvents = summary.storedEvents;
    vector.pressCount = summary.pressCount;
    vector.releaseCount = summary.releaseCount;
    vector.ignoredAutoRepeatCount = summary.ignoredAutoRepeatCount;
    vector.overlapPressCount = summary.overlapPressCount;
    vector.unmatchedReleaseCount = summary.unmatchedReleaseCount;
    vector.keysStillPressedCount = summary.keysStillPressedCount;

    vector.durationMs = static_cast<double>(summary.durationNs) / 1000000.0;

    vector.averageDwellMs = summary.averageDwellMs;
    vector.minDwellMs = summary.minDwellMs;
    vector.maxDwellMs = summary.maxDwellMs;

    vector.averageFlightMs = summary.averageFlightMs;
    vector.minFlightMs = summary.minFlightMs;
    vector.maxFlightMs = summary.maxFlightMs;

    if (summary.pressCount > 0)
    {
        vector.overlapRatio =
            static_cast<double>(summary.overlapPressCount) / summary.pressCount;
    }

    if (summary.releaseCount > 0)
    {
        vector.unmatchedReleaseRatio =
            static_cast<double>(summary.unmatchedReleaseCount) / summary.releaseCount;
    }

    const int observedEventCount =
        summary.storedEvents + summary.ignoredAutoRepeatCount;

    if (observedEventCount > 0)
    {
        vector.ignoredRepeatRatio =
            static_cast<double>(summary.ignoredAutoRepeatCount) / observedEventCount;
    }

    return vector;
}
