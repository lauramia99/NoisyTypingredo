#include "featureextractor.h"
#include "mainwindow.h"
#include "sessionfeaturecsvwriter.h"
#include "typingtextedit.h"
#include "sessioneventcsvwriter.h"
#include "profilemodel.h"


#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
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


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("NoisyTyping");
    resize(900, 600);

    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);

    saveSessionButton_ = new QPushButton("Save Session", centralWidget);
    resetSessionButton_ = new QPushButton("Reset Session", centralWidget);
    buildProfileButton_ = new QPushButton("Build Profile", centralWidget);

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
    consentCheckBox_ = new QCheckBox("Consent confirmed", centralWidget);
    consentCheckBox_->setToolTip("Required before saving a typing sample.");

    auto *controlsLayout = new QHBoxLayout();
    controlsLayout->addWidget(participantIdLabel);
    controlsLayout->addWidget(participantIdEdit_, 1);
    controlsLayout->addWidget(samplePurposeLabel);
    controlsLayout->addWidget(samplePurposeCombo_);
    controlsLayout->addWidget(textModeLabel);
    controlsLayout->addWidget(textModeCombo_);
    controlsLayout->addWidget(promptLabelLabel);
    controlsLayout->addWidget(promptLabelEdit_, 1);
    controlsLayout->addWidget(consentCheckBox_);
    controlsLayout->addWidget(saveSessionButton_);
    controlsLayout->addWidget(resetSessionButton_);
    controlsLayout->addWidget(buildProfileButton_);
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

    connect(buildProfileButton_, &QPushButton::clicked,
            this, &MainWindow::buildCurrentParticipantProfile);

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

    if (!databaseManager_.open())
    {
        QMessageBox::warning(
            this,
            "Database Initialization Failed",
            QString("Could not initialize SQLite database:\n%1")
                .arg(databaseManager_.lastErrorText()));
    }
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

void MainWindow::updateSessionStatus()
{
    const SessionSummary summary =
        FeatureExtractor::buildSessionSummary(currentSession_);
    const SessionFeatureVector featureVector =
        FeatureExtractor::buildFeatureVector(currentSession_, summary);

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

void MainWindow::saveCurrentSession()
{
    const SessionSummary summary =
        FeatureExtractor::buildSessionSummary(currentSession_);

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

    if (!consentCheckBox_->isChecked())
    {
        QMessageBox::warning(this, "Consent Required",
                             "Please confirm consent before saving the typing sample.");
        consentCheckBox_->setFocus();
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

    const SessionFeatureVector featureVector =
        FeatureExtractor::buildFeatureVector(currentSession_, summary);
    const QString featureFilePath = SessionFeatureCsvWriter::defaultFilePath();

    if (!SessionFeatureCsvWriter::appendRow(featureFilePath, featureVector))
    {
        QMessageBox::warning(this, "Save Failed",
                             "Could not append the feature vector to the CSV file.");
        return;
    }

    if (!databaseManager_.saveSession(currentSession_, summary, featureVector))
    {
        QMessageBox::warning(
            this,
            "Database Save Failed",
            QString("Could not save session to SQLite:\n%1")
                .arg(databaseManager_.lastErrorText()));
        return;
    }

    SavedSessionCheck savedCheck;
    const QString sessionId = currentSession_.id.toString(QUuid::WithoutBraces);

    if (!databaseManager_.loadSavedSessionCheck(sessionId, savedCheck))
    {
        QMessageBox::warning(
            this,
            "Database Verification Failed",
            QString("Could not verify saved SQLite data:\n%1")
                .arg(databaseManager_.lastErrorText()));
        return;
    }

    if (savedCheck.storedEventCount != currentSession_.events.size())
    {
        QMessageBox::warning(
            this,
            "Database Verification Failed",
            QString("Saved event count mismatch.\nExpected: %1\nSaved: %2")
                .arg(currentSession_.events.size())
                .arg(savedCheck.storedEventCount));
        return;
    }

    if (!savedCheck.hasFeatureRow)
    {
        QMessageBox::warning(
            this,
            "Database Verification Failed",
            "The feature row was not found in SQLite.");
        return;
    }

    DatabaseStats databaseStats;

    if (!databaseManager_.loadDatabaseStats(databaseStats))
    {
        QMessageBox::warning(
            this,
            "Database Statistics Failed",
            QString("Could not load SQLite statistics:\n%1")
                .arg(databaseManager_.lastErrorText()));
        return;
    }

    QMessageBox::information(
        this,
        "Session Saved",
        QString(
            "Feature vector saved to:\n%1\n\n"
            "Raw session saved to:\n%2\n\n"
            "SQLite events verified: %3\n\n"
            "Database totals:\n"
            "Participants: %4\n"
            "Sessions: %5\n"
            "Events: %6\n"
            "Feature rows: %7")
            .arg(QDir::toNativeSeparators(featureFilePath))
            .arg(QDir::toNativeSeparators(rawSessionFilePath))
            .arg(savedCheck.storedEventCount)
            .arg(databaseStats.participantCount)
            .arg(databaseStats.sessionCount)
            .arg(databaseStats.eventCount)
            .arg(databaseStats.featureRowCount)
        );

    typingArea_->clear();
    consentCheckBox_->setChecked(false);
    startNewSession();
    updateSessionStatus();
    typingArea_->setFocus();
}

void MainWindow::resetCurrentSession()
{
    typingArea_->clear();
    consentCheckBox_->setChecked(false);
    startNewSession();
    updateSessionStatus();
    typingArea_->setFocus();
}

void MainWindow::buildCurrentParticipantProfile()
{
    const QString participantId = participantIdEdit_->text().trimmed();

    if (participantId.isEmpty())
    {
        QMessageBox::warning(this, "Missing Participant ID",
                             "Please enter a participant ID before building a profile.");
        participantIdEdit_->setFocus();
        return;
    }

    QVector<SessionFeatureVector> trainingSamples;

    if (!databaseManager_.loadTrainingFeatureVectors(participantId, trainingSamples))
    {
        QMessageBox::warning(
            this,
            "Profile Build Failed",
            QString("Could not load training samples:\n%1")
                .arg(databaseManager_.lastErrorText()));
        return;
    }

    if (trainingSamples.isEmpty())
    {
        QMessageBox::information(
            this,
            "No Training Data",
            QString("No training samples found for participant '%1'.")
                .arg(participantId));
        return;
    }

    const UserProfile profile =
        ProfileModel::buildProfile(participantId, trainingSamples);

    QMessageBox::information(
        this,
        "Profile Built",
        QString(
            "Participant: %1\n"
            "Training sessions: %2\n\n"
            "Dwell mean: %3 ms\n"
            "Dwell stddev: %4 ms\n\n"
            "Flight mean: %5 ms\n"
            "Flight stddev: %6 ms")
            .arg(profile.participantId)
            .arg(profile.trainingSessionCount)
            .arg(profile.averageDwellMsMean, 0, 'f', 2)
            .arg(profile.averageDwellMsStdDev, 0, 'f', 2)
            .arg(profile.averageFlightMsMean, 0, 'f', 2)
            .arg(profile.averageFlightMsStdDev, 0, 'f', 2));
}

