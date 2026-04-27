#include "mainwindow.h"
#include "typingtextedit.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QDateTime>
#include <QUuid>
#include <QStatusBar>
#include <QString>
#include <QHash>

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


    connect(typingArea_, &typingtextedit::keyPressed, this,
            &MainWindow::handleKeyPressed);

    connect(typingArea_, &typingtextedit::keyReleased, this,
            &MainWindow::handleKeyReleased);

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

void MainWindow::appendEvent(KeyAction action, int key, qint64 timestampNs, bool autoRepeat)
{
    if (autoRepeat)
    {
        ++currentSession_.ignoredAutoRepeatCount;
        return;
    }
    KeystrokeEvent event{action, key, timestampNs, autoRepeat};
    currentSession_.events.append(event);
}

void MainWindow::handleKeyPressed(int key, qint64 timestampNs, bool autoRepeat)
{
    appendEvent(KeyAction::Press, key, timestampNs, autoRepeat);
    updateSessionStatus();
}

void MainWindow::handleKeyReleased(int key, qint64 timestampNs, bool autoRepeat)
{
    appendEvent(KeyAction::Release, key, timestampNs, autoRepeat);
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
    return summary;
}

void MainWindow::updateSessionStatus()
{
    const SessionSummary summary = buildSessionSummary();

    statusBar()->showMessage(
        QString("Stored: %1 | Press: %2 | Release: %3 | Dwell avg: %4 ms | Dwell samples: %5 | Open keys: %6 | Ignored repeats: %7")
            .arg(summary.storedEvents)
            .arg(summary.pressCount)
            .arg(summary.releaseCount)
            .arg(summary.averageDwellMs, 0, 'f', 2)
            .arg(summary.dwellSampleCount)
            .arg(summary.keysStillPressedCount)
            .arg(summary.ignoredAutoRepeatCount));

}

void MainWindow::fillDwellStats(SessionSummary &summary) const
{
    QHash<int, qint64> pressedKeys;
    qint64 totalDwellNs = 0;
    bool hasDwellSample = false;
    qint64 minDwellNs = 0;
    qint64 maxDwellNs = 0;

    for (const KeystrokeEvent &event : currentSession_.events)
    {
        if (event.action == KeyAction::Press)
        {
            pressedKeys[event.key] = event.timestampNs;
            continue;
        }

        auto it = pressedKeys.find(event.key);
        if (it == pressedKeys.end())
        {
            ++summary.unmatchedReleaseCount;
            continue;
        }

        const qint64 dwellNs = event.timestampNs - it.value();
        pressedKeys.erase(it);

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

    summary.keysStillPressedCount = pressedKeys.size();

    if (summary.dwellSampleCount > 0)
    {
        summary.averageDwellMs =
            static_cast<double>(totalDwellNs) / summary.dwellSampleCount / 1000000.0;
        summary.minDwellMs = static_cast<double>(minDwellNs) / 1000000.0;
        summary.maxDwellMs = static_cast<double>(maxDwellNs) / 1000000.0;
    }
}







