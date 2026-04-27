#include "mainwindow.h"
#include "typingtextedit.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QDateTime>
#include <QUuid>
#include <QStatusBar>
#include <QString>
#include <QHash>

static quint64 makePhysicalKeyId(const KeystrokeEvent &event)
{
    if (event.nativeScanCode != 0)
    {
        return (static_cast<quint64>(1) << 32) | event.nativeScanCode;
    }

    return (static_cast<quint64>(2) << 32) | static_cast<quint32>(event.key);
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("NoisyTyping");
    resize(900, 600);

    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);

    typingArea_ = new typingtextedit(centralWidget);
    typingArea_->setPlaceholderText("Start typing...");

    layout->addWidget(typingArea_);
    setCentralWidget(centralWidget);


    connect(typingArea_, &typingtextedit::keystrokeCaptured, this,
            &MainWindow::handleCapturedKeystroke);


    startNewSession();

    updateSessionStatus();

}

void MainWindow::startNewSession()
{
    currentSession_.id = QUuid::createUuid();
    currentSession_.startedAtUtc = QDateTime::currentDateTimeUtc();
    currentSession_.events.clear();
    currentSession_.ignoredAutoRepeatCount = 0;
}

void MainWindow::appendEvent(const KeystrokeEvent &event)
{
    if (event.autoRepeat)
    {
        ++currentSession_.ignoredAutoRepeatCount;
        return;
    }
    currentSession_.events.append(event);
}

void MainWindow::handleCapturedKeystroke(const KeystrokeEvent &event)
{
    appendEvent(event);
    updateSessionStatus();
}

SessionSummary MainWindow::buildSessionSummary() const
{
    SessionSummary summary;
    summary.storedEvents = currentSession_.events.size();
    summary.ignoredAutoRepeatCount = currentSession_.ignoredAutoRepeatCount;

    for (const KeystrokeEvent &event : currentSession_.events)
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

    if (!currentSession_.events.isEmpty())
    {
        summary.durationNs =
            currentSession_.events.last().timestampNs -
            currentSession_.events.first().timestampNs;
    }

    fillDwellStats(summary);
    fillFlightStats(summary);
    return summary;
}

void MainWindow::updateSessionStatus()
{
    const SessionSummary summary = buildSessionSummary();

    statusBar()->showMessage(
        QString("Stored: %1 | Press: %2 | Release: %3 | Dwell avg: %4 ms | Flight avg: %5 ms | Overlaps: %6 | Open keys: %7 | Ignored repeats: %8")
            .arg(summary.storedEvents)
            .arg(summary.pressCount)
            .arg(summary.releaseCount)
            .arg(summary.averageDwellMs, 0, 'f', 2)
            .arg(summary.averageFlightMs, 0, 'f', 2)
            .arg(summary.overlapPressCount)
            .arg(summary.keysStillPressedCount)
            .arg(summary.ignoredAutoRepeatCount));


}

void MainWindow::fillDwellStats(SessionSummary &summary) const
{
    QHash<quint64, QVector<qint64>> pressedKeys;
    qint64 totalDwellNs = 0;
    bool hasDwellSample = false;
    qint64 minDwellNs = 0;
    qint64 maxDwellNs = 0;

    for (const KeystrokeEvent &event : currentSession_.events)
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

void MainWindow::fillFlightStats(SessionSummary &summary) const
{
    bool hasPreviousPress = false;
    qint64 previousPressTimestampNs = 0;
    qint64 totalFlightNs = 0;
    bool hasFlightSample = false;
    qint64 minFlightNs = 0;
    qint64 maxFlightNs = 0;
    QHash<quint64, int> activePressCounts;
    int activePressedKeyCount = 0;

    for (const KeystrokeEvent &event : currentSession_.events)
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



