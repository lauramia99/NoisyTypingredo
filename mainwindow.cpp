#include "mainwindow.h"
#include "sessionfeaturecsvwriter.h"
#include "typingtextedit.h"
#include "sessioneventcsvwriter.h"


#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QString>
#include <QUuid>
#include <QVBoxLayout>
#include <QWidget>

// Prefer physical key identity when available so overlap handling is stable.

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

    auto *participantIdLabel = new QLabel("Participant ID:", centralWidget);
    participantIdEdit_ = new QLineEdit(centralWidget);
    participantIdEdit_->setPlaceholderText("e.g. user_01");

    auto *samplePurposeLabel = new QLabel("Sample Purpose:", centralWidget);
    samplePurposeCombo_ = new QComboBox(centralWidget);
    samplePurposeCombo_->addItem("training");
    samplePurposeCombo_->addItem("verification");

    auto *textModeLabel = new QLabel("Text Mode:", centralWidget);
    textModeCombo_ = new QComboBox(centralWidget);
    textModeCombo_->addItem("free_text");
    textModeCombo_->addItem("fixed_text");

    auto *promptLabelLabel = new QLabel("Prompt Label:", centralWidget);
    promptLabelEdit_ = new QLineEdit(centralWidget);
    promptLabelEdit_->setPlaceholderText("optional, e.g. paragraph_01");

    auto *controlsLayout = new QHBoxLayout();
    controlsLayout->addWidget(participantIdLabel);
    controlsLayout->addWidget(participantIdEdit_, 1);
    controlsLayout->addWidget(samplePurposeLabel);
    controlsLayout->addWidget(samplePurposeCombo_);
    controlsLayout->addWidget(textModeLabel);
    controlsLayout->addWidget(textModeCombo_);
    controlsLayout->addWidget(promptLabelLabel);
    controlsLayout->addWidget(promptLabelEdit_, 1);
    controlsLayout->addWidget(saveSessionButton_);
    controlsLayout->addWidget(resetSessionButton_);
    controlsLayout->addStretch();

    typingArea_ = new typingtextedit(centralWidget);
    typingArea_->setPlaceholderText("Start typing...");

    layout->addLayout(controlsLayout);
    layout->addWidget(typingArea_);

    setCentralWidget(centralWidget);

    connect(saveSessionButton_, &QPushButton::clicked,
            this, &MainWindow::saveCurrentSession);

    connect(resetSessionButton_, &QPushButton::clicked,
            this, &MainWindow::resetCurrentSession);

    connect(typingArea_, &typingtextedit::keystrokeCaptured, this,
            &MainWindow::handleCapturedKeystroke);

    const auto refreshSessionMetadata = [this]() {
        syncSessionMetadataFromUi();
        updateSessionStatus();
    };

    connect(participantIdEdit_, &QLineEdit::textChanged, this,
            [refreshSessionMetadata](const QString &) {
                refreshSessionMetadata();
            });

    connect(samplePurposeCombo_, &QComboBox::currentTextChanged, this,
            [refreshSessionMetadata](const QString &) {
                refreshSessionMetadata();
            });

    connect(textModeCombo_, &QComboBox::currentTextChanged, this,
            [refreshSessionMetadata](const QString &) {
                refreshSessionMetadata();
            });

    connect(promptLabelEdit_, &QLineEdit::textChanged, this,
            [refreshSessionMetadata](const QString &) {
                refreshSessionMetadata();
            });

    startNewSession();
    updateSessionStatus();
}

void MainWindow::syncSessionMetadataFromUi()
{
    currentSession_.participantId = participantIdEdit_->text().trimmed();
    currentSession_.samplePurpose = samplePurposeCombo_->currentText().trimmed();
    currentSession_.textMode = textModeCombo_->currentText().trimmed();
    currentSession_.promptLabel = promptLabelEdit_->text().trimmed();
}

void MainWindow::startNewSession()
{
    currentSession_.id = QUuid::createUuid();
    currentSession_.startedAtUtc = QDateTime::currentDateTimeUtc();
    currentSession_.events.clear();
    currentSession_.ignoredAutoRepeatCount = 0;
    syncSessionMetadataFromUi();
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

    vector.participantId = currentSession_.participantId;

    vector.samplePurpose = currentSession_.samplePurpose;
    vector.textMode = currentSession_.textMode;
    vector.promptLabel = currentSession_.promptLabel;

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
        QString("User: %1 | Purpose: %2 | Mode: %3 | Stored: %4 | Dwell avg: %5 ms | Flight avg: %6 ms | Open keys: %7 | Repeat: %8%")
            .arg(featureVector.participantId.isEmpty() ? "-" : featureVector.participantId)
            .arg(featureVector.samplePurpose.isEmpty() ? "-" : featureVector.samplePurpose)
            .arg(featureVector.textMode.isEmpty() ? "-" : featureVector.textMode)
            .arg(featureVector.storedEvents)
            .arg(featureVector.averageDwellMs, 0, 'f', 2)
            .arg(featureVector.averageFlightMs, 0, 'f', 2)
            .arg(featureVector.keysStillPressedCount)
            .arg(featureVector.ignoredRepeatRatio * 100.0, 0, 'f', 1));
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

    if (currentSession_.participantId.isEmpty())
    {
        syncSessionMetadataFromUi();
    }

    if (currentSession_.participantId.isEmpty())
    {
        QMessageBox::warning(this, "Missing Participant ID",
                             "Please enter a participant ID before saving the session.");
        participantIdEdit_->setFocus();
        return;
    }

    if (summary.storedEvents == 0)
    {
        QMessageBox::information(this, "No Data",
                                 "There is no captured session data to save.");
        return;
    }

    const QString rawSessionFilePath =
        SessionEventCsvWriter::sessionFilePath(currentSession_);

    if (!SessionEventCsvWriter::writeSession(rawSessionFilePath, currentSession_))
    {
        QMessageBox::warning(this, "Save Failed",
                             "Could not write the raw session events CSV file.");
        return;
    }


    const SessionFeatureVector featureVector = buildFeatureVector(summary);
    const QString featureFilePath = SessionFeatureCsvWriter::defaultFilePath();

    if (!SessionFeatureCsvWriter::appendRow(featureFilePath, featureVector))
    {
        QMessageBox::warning(this, "Save Failed",
                             "Could not append the feature vector to the CSV file.");
        return;
    }

    QMessageBox::information(
        this,
        "Session Saved",
        QString("Feature vector saved to:\n%1\n\nRaw session saved to:\n%2")
            .arg(QDir::toNativeSeparators(featureFilePath))
            .arg(QDir::toNativeSeparators(rawSessionFilePath)));


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

