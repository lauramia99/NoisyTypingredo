#include "mainwindow.h"
#include "typingtextedit.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QDateTime>
#include <QUuid>
#include <QStatusBar>
#include <QString>
#include <QHash>

#include "sessionfeaturecsvwriter.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>


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

    saveSessionButton_ = new QPushButton("Save Session", centralWidget);
    resetSessionButton_ = new QPushButton("Reset Session", centralWidget);

    auto *controlsLayout = new QHBoxLayout();
    controlsLayout->addWidget(saveSessionButton_);
    controlsLayout->addWidget(resetSessionButton_);
    controlsLayout->addStretch();

    typingArea_ = new typingtextedit(centralWidget);
    typingArea_->setPlaceholderText("Start typing...");

    layout->addLayout(controlsLayout);
    layout->addWidget(typingArea_);

    layout->addWidget(typingArea_);
    setCentralWidget(centralWidget);

    connect(saveSessionButton_, &QPushButton::clicked,
            this, &MainWindow::saveCurrentSession);

    connect(resetSessionButton_, &QPushButton::clicked,
            this, &MainWindow::resetCurrentSession);

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

SessionFeatureVector MainWindow::buildFeatureVector(const SessionSummary &summary) const
{
    SessionFeatureVector vector;

    vector.sessionId = currentSession_.id.toString(QUuid::WithoutBraces);
    vector.startedAtUtcIso = currentSession_.startedAtUtc.toString(Qt::ISODateWithMs);

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


void MainWindow::updateSessionStatus()
{
    const SessionSummary summary = buildSessionSummary();
    const SessionFeatureVector featureVector = buildFeatureVector(summary);


    statusBar()->showMessage(
        QString("Stored: %1 | Dwell avg: %2 ms | Flight avg: %3 ms | Overlap: %4% | Repeat: %5% | Open keys: %6")
            .arg(featureVector.storedEvents)
            .arg(featureVector.averageDwellMs, 0, 'f', 2)
            .arg(featureVector.averageFlightMs, 0, 'f', 2)
            .arg(featureVector.overlapRatio * 100.0, 0, 'f', 1)
            .arg(featureVector.ignoredRepeatRatio * 100.0, 0, 'f', 1)
            .arg(featureVector.keysStillPressedCount));

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

void MainWindow::saveCurrentSession()
{
    const SessionSummary summary = buildSessionSummary();

    if (summary.storedEvents == 0)
    {
        QMessageBox::information(this, "No Data",
                                 "There is no captured session data to save.");
        return;
    }

    const SessionFeatureVector featureVector = buildFeatureVector(summary);
    const QString filePath = SessionFeatureCsvWriter::defaultFilePath();

    if (!SessionFeatureCsvWriter::appendRow(filePath, featureVector))
    {
        QMessageBox::warning(this, "Save Failed",
                             "Could not append the feature vector to the CSV file.");
        return;
    }

    typingArea_->clear();
    startNewSession();
    updateSessionStatus();
    typingArea_->setFocus();
}

void MainWindow::resetCurrentSession()
{
    typingArea_->clear();
    startNewSession();
    updateSessionStatus();
    typingArea_->setFocus();
}



