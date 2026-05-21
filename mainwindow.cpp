#include "mainwindow.h"

#include "featureextractor.h"
#include "profilemodel.h"
#include "promptlibrary.h"
#include "sessioneventcsvwriter.h"
#include "sessionfeaturecsvwriter.h"
#include "thresholdanalysis.h"
#include "thresholdanalysiscsvwriter.h"
#include "typingtextedit.h"
#include "verificationresultscsvwriter.h"
#include "verificationstatscsvwriter.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDateTime>
#include <QDir>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("NoisyTyping");
    resize(980, 720);
    setMinimumSize(760, 560);

    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);

    saveSessionButton_ = new QPushButton("Save Session", centralWidget);
    resetSessionButton_ = new QPushButton("Reset Session", centralWidget);
    buildProfileButton_ = new QPushButton("Build Profile", centralWidget);
    verifySessionButton_ = new QPushButton("Verify Session", centralWidget);
    verificationStatsButton_ = new QPushButton("Verification Stats", centralWidget);
    thresholdAnalysisButton_ = new QPushButton("Threshold Analysis", centralWidget);
    startEnrollmentButton_ = new QPushButton("Start Enrollment", centralWidget);
    saveAndNextEnrollmentButton_ = new QPushButton("Save && Next", centralWidget);
    saveAndNextEnrollmentButton_->setEnabled(false);

    auto *participantIdLabel = new QLabel("Participant ID:", centralWidget);
    participantIdCombo_ = new QComboBox(centralWidget);
    participantIdCombo_->setEditable(true);
    participantIdCombo_->setInsertPolicy(QComboBox::NoInsert);
    participantIdCombo_->setMaxVisibleItems(12);
    participantIdCombo_->lineEdit()->setPlaceholderText("select or type participant ID");

    participantIdCombo_->completer()->setCompletionMode(QCompleter::PopupCompletion);
    participantIdCombo_->completer()->setFilterMode(Qt::MatchContains);
    participantIdCombo_->completer()->setCaseSensitivity(Qt::CaseInsensitive);

    auto *samplePurposeLabel = new QLabel("Sample Purpose:", centralWidget);
    samplePurposeCombo_ = new QComboBox(centralWidget);
    samplePurposeCombo_->addItem("training");
    samplePurposeCombo_->addItem("verification");

    auto *textModeLabel = new QLabel("Text Mode:", centralWidget);
    textModeCombo_ = new QComboBox(centralWidget);
    textModeCombo_->addItem("free_text");
    textModeCombo_->addItem("fixed_text");

    auto *attemptTypeLabel = new QLabel("Attempt Type:", centralWidget);
    attemptTypeCombo_ = new QComboBox(centralWidget);
    attemptTypeCombo_->addItem("genuine");
    attemptTypeCombo_->addItem("impostor");

    auto *promptLabelLabel = new QLabel("Prompt Label:", centralWidget);
    promptLabelEdit_ = new QLineEdit(centralWidget);
    promptLabelEdit_->setReadOnly(true);

    auto *promptSelectorLabel = new QLabel("Prompt:", centralWidget);
    promptCombo_ = new QComboBox(centralWidget);

    promptDisplay_ = new QPlainTextEdit(centralWidget);
    promptDisplay_->setReadOnly(true);
    promptDisplay_->setMaximumHeight(90);

    consentCheckBox_ = new QCheckBox("Consent confirmed", centralWidget);
    consentCheckBox_->setToolTip("Required before saving a typing sample.");

    enrollmentStatusLabel_ = new QLabel("Enrollment: select or enter participant", centralWidget);
    enrollmentStatusLabel_->setWordWrap(true);

    enrollmentProgressLabel_ = new QLabel("Enrollment flow: not started", centralWidget);
    enrollmentProgressLabel_->setWordWrap(true);

    auto *thresholdLabel = new QLabel("Threshold:", centralWidget);
    verificationThresholdSpinBox_ = new QDoubleSpinBox(centralWidget);
    verificationThresholdSpinBox_->setRange(0.0, 100.0);
    verificationThresholdSpinBox_->setDecimals(2);
    verificationThresholdSpinBox_->setSingleStep(0.25);
    verificationThresholdSpinBox_->setValue(3.0);
    verificationThresholdSpinBox_->setToolTip("Verification threshold. Lower is stricter.");

    auto *sessionGroup = new QGroupBox("Session metadata", centralWidget);
    auto *sessionLayout = new QGridLayout(sessionGroup);
    sessionLayout->addWidget(participantIdLabel, 0, 0);
    sessionLayout->addWidget(participantIdCombo_, 0, 1);
    sessionLayout->addWidget(samplePurposeLabel, 0, 2);
    sessionLayout->addWidget(samplePurposeCombo_, 0, 3);
    sessionLayout->addWidget(textModeLabel, 1, 0);
    sessionLayout->addWidget(textModeCombo_, 1, 1);
    sessionLayout->addWidget(attemptTypeLabel, 1, 2);
    sessionLayout->addWidget(attemptTypeCombo_, 1, 3);
    sessionLayout->addWidget(promptSelectorLabel, 2, 0);
    sessionLayout->addWidget(promptCombo_, 2, 1);
    sessionLayout->addWidget(promptLabelLabel, 2, 2);
    sessionLayout->addWidget(promptLabelEdit_, 2, 3);
    sessionLayout->addWidget(promptDisplay_, 3, 0, 1, 4);
    sessionLayout->addWidget(enrollmentStatusLabel_, 4, 0, 1, 4);
    sessionLayout->addWidget(enrollmentProgressLabel_, 5, 0, 1, 4);
    sessionLayout->setColumnStretch(1, 1);
    sessionLayout->setColumnStretch(3, 1);

    auto *actionsGroup = new QGroupBox("Controls", centralWidget);
    auto *actionsLayout = new QGridLayout(actionsGroup);
    actionsLayout->addWidget(consentCheckBox_, 0, 0);
    actionsLayout->addWidget(thresholdLabel, 0, 1);
    actionsLayout->addWidget(verificationThresholdSpinBox_, 0, 2);
    actionsLayout->addWidget(saveSessionButton_, 1, 0);
    actionsLayout->addWidget(resetSessionButton_, 1, 1);
    actionsLayout->addWidget(buildProfileButton_, 1, 2);
    actionsLayout->addWidget(verifySessionButton_, 1, 3);
    actionsLayout->addWidget(verificationStatsButton_, 2, 0, 1, 2);
    actionsLayout->addWidget(thresholdAnalysisButton_, 2, 2, 1, 2);
    actionsLayout->addWidget(startEnrollmentButton_, 3, 0, 1, 2);
    actionsLayout->addWidget(saveAndNextEnrollmentButton_, 3, 2, 1, 2);
    actionsLayout->setColumnStretch(4, 1);

    typingArea_ = new typingtextedit(centralWidget);
    typingArea_->setPlaceholderText("Type the selected prompt here...");

    layout->addWidget(sessionGroup);
    layout->addWidget(actionsGroup);
    layout->addWidget(typingArea_, 1);

    setCentralWidget(centralWidget);

    connect(saveSessionButton_, &QPushButton::clicked,
            this, &MainWindow::saveCurrentSession);

    connect(resetSessionButton_, &QPushButton::clicked,
            this, &MainWindow::resetCurrentSession);

    connect(buildProfileButton_, &QPushButton::clicked,
            this, &MainWindow::buildCurrentParticipantProfile);

    connect(typingArea_, &typingtextedit::keystrokeCaptured, this,
            &MainWindow::handleCapturedKeystroke);

    connect(verifySessionButton_, &QPushButton::clicked,
            this, &MainWindow::verifyCurrentSession);

    connect(verificationStatsButton_, &QPushButton::clicked,
            this, &MainWindow::showVerificationStats);

    connect(thresholdAnalysisButton_, &QPushButton::clicked,
            this, &MainWindow::analyzeThresholds);

    connect(startEnrollmentButton_, &QPushButton::clicked,
        this, &MainWindow::startEnrollment);

    connect(saveAndNextEnrollmentButton_, &QPushButton::clicked,
        this, &MainWindow::saveEnrollmentStep);

    const auto refreshSessionMetadata = [this]() {
        syncSessionMetadataFromUi();
        updateSessionStatus();
    };

    connect(participantIdCombo_, &QComboBox::currentTextChanged, this,
            [this, refreshSessionMetadata](const QString &) {
                refreshSessionMetadata();
                updateEnrollmentStatus();
            });

    connect(samplePurposeCombo_, &QComboBox::currentTextChanged, this,
            [refreshSessionMetadata](const QString &) {
                refreshSessionMetadata();
            });

    connect(textModeCombo_, &QComboBox::currentTextChanged, this,
            [refreshSessionMetadata](const QString &) {
                refreshSessionMetadata();
            });

    connect(promptCombo_, &QComboBox::currentTextChanged, this,
            [this, refreshSessionMetadata](const QString &) {
                updatePromptSelection();
                refreshSessionMetadata();
            });

    populatePromptSelector();
    updatePromptSelection();

    if (!databaseManager_.open())
    {
        QMessageBox::warning(
            this,
            "Database Initialization Failed",
            QString("Could not initialize SQLite database:\n%1")
                .arg(databaseManager_.lastErrorText()));
    }
    else
    {
        refreshParticipantList();
    }

    startNewSession();
    updateSessionStatus();
    updateEnrollmentStatus();
    updateEnrollmentControls();
}

QString MainWindow::currentParticipantId() const
{
    return participantIdCombo_->currentText().trimmed();
}

void MainWindow::refreshParticipantList()
{
    QStringList participantIds;

    if (!databaseManager_.loadParticipantIds(participantIds))
    {
        return;
    }

    const QString selectedParticipantId = currentParticipantId();
    const QSignalBlocker blocker(participantIdCombo_);

    participantIdCombo_->clear();
    participantIdCombo_->addItems(participantIds);
    participantIdCombo_->setCurrentText(selectedParticipantId);
}

void MainWindow::populatePromptSelector()
{
    promptCombo_->clear();

    const QVector<PromptDefinition> prompts =
        PromptLibrary::fixedTextPrompts();

    for (const PromptDefinition &prompt : prompts)
    {
        promptCombo_->addItem(prompt.title, prompt.label);
    }
}

void MainWindow::updatePromptSelection()
{
    const QString promptLabel =
        promptCombo_->currentData().toString();

    const PromptDefinition prompt =
        PromptLibrary::promptByLabel(promptLabel);

    promptLabelEdit_->setText(prompt.label);
    promptDisplay_->setPlainText(prompt.text);

    textModeCombo_->setCurrentText("fixed_text");
}

void MainWindow::updateEnrollmentStatus()
{
    const QString participantId = currentParticipantId();

    if (participantId.isEmpty())
    {
        enrollmentStatusLabel_->setText("Enrollment: select or enter participant");
        return;
    }

    EnrollmentStatus status;

    if (!databaseManager_.loadEnrollmentStatus(participantId, status))
    {
        enrollmentStatusLabel_->setText("Enrollment: unavailable");
        return;
    }

    QStringList promptParts;

    for (const EnrollmentPromptStatus &promptStatus : status.prompts)
    {
        const int shownCompleted =
            promptStatus.completedCount > promptStatus.requiredCount
                ? promptStatus.requiredCount
                : promptStatus.completedCount;

        promptParts << QString("%1 %2/%3")
                           .arg(promptStatus.promptLabel)
                           .arg(shownCompleted)
                           .arg(promptStatus.requiredCount);
    }

    enrollmentStatusLabel_->setText(
        QString("Enrollment: %1/%2 %3 | %4")
            .arg(status.completedTotal)
            .arg(status.requiredTotal)
            .arg(status.isComplete ? "complete" : "incomplete")
            .arg(promptParts.join(", ")));
}

bool MainWindow::ensureEnrollmentComplete(const QString &participantId)
{
    EnrollmentStatus status;

    if (!databaseManager_.loadEnrollmentStatus(participantId, status))
    {
        QMessageBox::warning(
            this,
            "Enrollment Check Failed",
            QString("Could not load enrollment status:\n%1")
                .arg(databaseManager_.lastErrorText()));
        return false;
    }

    updateEnrollmentStatus();

    if (status.isComplete)
    {
        return true;
    }

    QStringList missingParts;

    for (const EnrollmentPromptStatus &promptStatus : status.prompts)
    {
        if (promptStatus.completedCount < promptStatus.requiredCount)
        {
            missingParts << QString("%1: %2 more")
                                .arg(promptStatus.promptLabel)
                                .arg(promptStatus.requiredCount - promptStatus.completedCount);
        }
    }

    QMessageBox::information(
        this,
        "Enrollment Incomplete",
        QString(
            "Participant '%1' is not fully enrolled yet.\n\n"
            "Enrollment: %2/%3\n"
            "Missing:\n%4")
            .arg(participantId)
            .arg(status.completedTotal)
            .arg(status.requiredTotal)
            .arg(missingParts.join("\n")));

    return false;
}

void MainWindow::syncSessionMetadataFromUi()
{
    currentSession_.participantId = currentParticipantId();
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

bool MainWindow::saveCurrentSessionToStorage(bool showSuccessMessage,
                                             bool resetConsentAfterSave)
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
        participantIdCombo_->setFocus();
        return false;
    }

    if (!consentCheckBox_->isChecked())
    {
        QMessageBox::warning(this, "Consent Required",
                             "Please confirm consent before saving the typing sample.");
        consentCheckBox_->setFocus();
        return false;
    }

    if (summary.storedEvents == 0)
    {
        QMessageBox::information(this, "No Data",
                                 "There is no captured session data to save.");
        return false;
    }

    const QString rawSessionFilePath =
        SessionEventCsvWriter::sessionFilePath(currentSession_);

    if (!SessionEventCsvWriter::writeSession(rawSessionFilePath, currentSession_))
    {
        QMessageBox::warning(this, "Save Failed",
                             "Could not write the raw session events CSV file.");
        return false;
    }

    const SessionFeatureVector featureVector =
        FeatureExtractor::buildFeatureVector(currentSession_, summary);
    const QString featureFilePath = SessionFeatureCsvWriter::defaultFilePath();

    if (!SessionFeatureCsvWriter::appendRow(featureFilePath, featureVector))
    {
        QMessageBox::warning(this, "Save Failed",
                             "Could not append the feature vector to the CSV file.");
        return false;
    }

    if (!databaseManager_.saveSession(currentSession_, summary, featureVector))
    {
        QMessageBox::warning(
            this,
            "Database Save Failed",
            QString("Could not save session to SQLite:\n%1")
                .arg(databaseManager_.lastErrorText()));
        return false;
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
        return false;
    }

    if (savedCheck.storedEventCount != currentSession_.events.size())
    {
        QMessageBox::warning(
            this,
            "Database Verification Failed",
            QString("Saved event count mismatch.\nExpected: %1\nSaved: %2")
                .arg(currentSession_.events.size())
                .arg(savedCheck.storedEventCount));
        return false;
    }

    if (!savedCheck.hasFeatureRow)
    {
        QMessageBox::warning(
            this,
            "Database Verification Failed",
            "The feature row was not found in SQLite.");
        return false;
    }

    DatabaseStats databaseStats;

    if (!databaseManager_.loadDatabaseStats(databaseStats))
    {
        QMessageBox::warning(
            this,
            "Database Statistics Failed",
            QString("Could not load SQLite statistics:\n%1")
                .arg(databaseManager_.lastErrorText()));
        return false;
    }

    if (showSuccessMessage)
    {
        QMessageBox::information(
            this,
            "Session Saved",
            QString(
                "Feature vector saved to:\n%1\n\n"
                "Raw session saved to:\n%2\n\n"
                "SQLite database:\n%3\n\n"
                "SQLite events verified: %4\n\n"
                "Database totals:\n"
                "Participants: %5\n"
                "Sessions: %6\n"
                "Events: %7\n"
                "Feature rows: %8")
                .arg(QDir::toNativeSeparators(featureFilePath))
                .arg(QDir::toNativeSeparators(rawSessionFilePath))
                .arg(QDir::toNativeSeparators(databaseManager_.databaseFilePath()))
                .arg(savedCheck.storedEventCount)
                .arg(databaseStats.participantCount)
                .arg(databaseStats.sessionCount)
                .arg(databaseStats.eventCount)
                .arg(databaseStats.featureRowCount)
            );
    }

    typingArea_->clear();

    if (resetConsentAfterSave)
    {
        consentCheckBox_->setChecked(false);
    }

    refreshParticipantList();
    startNewSession();
    updateSessionStatus();
    updateEnrollmentStatus();
    typingArea_->setFocus();

    return true;
}

void MainWindow::saveCurrentSession()
{
    saveCurrentSessionToStorage(true, true);
}

void MainWindow::startEnrollment()
{
    const QString participantId = currentParticipantId();

    if (participantId.isEmpty())
    {
        QMessageBox::warning(this, "Missing Participant ID",
                             "Please enter a participant ID before enrollment.");
        participantIdCombo_->setFocus();
        return;
    }

    EnrollmentStatus status;

    if (!databaseManager_.loadEnrollmentStatus(participantId, status))
    {
        QMessageBox::warning(this, "Enrollment Failed",
                             databaseManager_.lastErrorText());
        return;
    }

    if (status.isComplete)
    {
        QMessageBox::information(
            this,
            "Enrollment Complete",
            "This participant already has the required fixed-text training samples.");
        return;
    }

    enrollmentPromptLabels_.clear();

    for (const EnrollmentPromptStatus &promptStatus : status.prompts)
    {
        const int remaining =
            promptStatus.requiredCount - promptStatus.completedCount;

        for (int i = 0; i < remaining; ++i)
        {
            enrollmentPromptLabels_.append(promptStatus.promptLabel);
        }
    }

    enrollmentActive_ = true;
    enrollmentStepIndex_ = 0;

    configureEnrollmentStep();
}

void MainWindow::saveEnrollmentStep()
{
    if (!enrollmentActive_)
    {
        return;
    }

    if (!saveCurrentSessionToStorage(false, false))
    {
        return;
    }

    ++enrollmentStepIndex_;

    if (enrollmentStepIndex_ >= enrollmentPromptLabels_.size())
    {
        enrollmentActive_ = false;
        updateEnrollmentControls();
        updateEnrollmentStatus();

        QMessageBox::information(
            this,
            "Enrollment Complete",
            "Required fixed-text enrollment samples are complete.");

        buildCurrentParticipantProfile();
        return;
    }

    configureEnrollmentStep();
}

void MainWindow::configureEnrollmentStep()
{
    if (!enrollmentActive_ ||
        enrollmentStepIndex_ >= enrollmentPromptLabels_.size())
    {
        updateEnrollmentControls();
        return;
    }

    const QString promptLabel =
        enrollmentPromptLabels_.at(enrollmentStepIndex_);

    const int promptIndex = promptCombo_->findData(promptLabel);

    if (promptIndex >= 0)
    {
        promptCombo_->setCurrentIndex(promptIndex);
    }

    samplePurposeCombo_->setCurrentText("training");
    textModeCombo_->setCurrentText("fixed_text");
    attemptTypeCombo_->setCurrentText("genuine");

    typingArea_->clear();
    startNewSession();
    updateSessionStatus();
    updateEnrollmentControls();
    typingArea_->setFocus();
}

void MainWindow::updateEnrollmentControls()
{
    startEnrollmentButton_->setEnabled(!enrollmentActive_);
    saveAndNextEnrollmentButton_->setEnabled(enrollmentActive_);

    participantIdCombo_->setEnabled(!enrollmentActive_);
    samplePurposeCombo_->setEnabled(!enrollmentActive_);
    textModeCombo_->setEnabled(!enrollmentActive_);
    attemptTypeCombo_->setEnabled(!enrollmentActive_);
    promptCombo_->setEnabled(!enrollmentActive_);

    saveSessionButton_->setEnabled(!enrollmentActive_);
    buildProfileButton_->setEnabled(!enrollmentActive_);
    verifySessionButton_->setEnabled(!enrollmentActive_);

    if (!enrollmentActive_)
    {
        enrollmentProgressLabel_->setText("Enrollment flow: not started");
        return;
    }

    enrollmentProgressLabel_->setText(
        QString("Enrollment flow: sample %1/%2 | %3")
            .arg(enrollmentStepIndex_ + 1)
            .arg(enrollmentPromptLabels_.size())
            .arg(enrollmentPromptLabels_.at(enrollmentStepIndex_)));
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
    const QString participantId = currentParticipantId();

    if (participantId.isEmpty())
    {
        QMessageBox::warning(this, "Missing Participant ID",
                             "Please enter a participant ID before building a profile.");
        participantIdCombo_->setFocus();
        return;
    }

    if (!ensureEnrollmentComplete(participantId))
    {
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

void MainWindow::verifyCurrentSession()
{
    const QString participantId = currentParticipantId();

    if (participantId.isEmpty())
    {
        QMessageBox::warning(this, "Missing Participant ID",
                             "Please enter a participant ID before verification.");
        participantIdCombo_->setFocus();
        return;
    }

    if (!ensureEnrollmentComplete(participantId))
    {
        return;
    }

    const SessionSummary currentSummary =
        FeatureExtractor::buildSessionSummary(currentSession_);

    if (currentSummary.storedEvents == 0)
    {
        QMessageBox::information(this, "No Session Data",
                                 "There is no current typing data to verify.");
        typingArea_->setFocus();
        return;
    }

    QVector<SessionFeatureVector> trainingSamples;

    if (!databaseManager_.loadTrainingFeatureVectors(participantId, trainingSamples))
    {
        QMessageBox::warning(
            this,
            "Verification Failed",
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

    const SessionFeatureVector currentFeatures =
        FeatureExtractor::buildFeatureVector(currentSession_, currentSummary);

    const double threshold = verificationThresholdSpinBox_->value();

    const VerificationDecision decision =
        ProfileModel::verifySample(profile, currentFeatures, threshold);

    VerificationResultRecord result;
    result.sessionId = currentSession_.id.toString(QUuid::WithoutBraces);
    result.participantId = participantId;
    result.attemptType = attemptTypeCombo_->currentText();
    result.totalScore = decision.score.totalScore;
    result.dwellDeviation = decision.score.dwellDeviation;
    result.flightDeviation = decision.score.flightDeviation;
    result.threshold = decision.threshold;
    result.accepted = decision.accepted;
    result.trainingSessionCount = profile.trainingSessionCount;

    if (!databaseManager_.saveVerificationResult(result))
    {
        QMessageBox::warning(
            this,
            "Verification Save Failed",
            QString("Could not save verification result:\n%1")
                .arg(databaseManager_.lastErrorText()));
        return;
    }

    QMessageBox::information(
        this,
        "Verification Result",
        QString(
            "Participant: %1\n"
            "Decision: %2\n"
            "Attempt type: %3\n\n"
            "Total score: %4\n"
            "Threshold: %5\n\n"
            "Dwell deviation: %6\n"
            "Flight deviation: %7\n\n"
            "Training sessions: %8\n\n"
            "Verification result saved to SQLite.")
            .arg(participantId)
            .arg(decision.accepted ? "ACCEPTED" : "REJECTED")
            .arg(attemptTypeCombo_->currentText())
            .arg(decision.score.totalScore, 0, 'f', 3)
            .arg(decision.threshold, 0, 'f', 3)
            .arg(decision.score.dwellDeviation, 0, 'f', 3)
            .arg(decision.score.flightDeviation, 0, 'f', 3)
            .arg(profile.trainingSessionCount));
}

void MainWindow::showVerificationStats()
{
    const QString participantId = currentParticipantId();

    if (participantId.isEmpty())
    {
        QMessageBox::warning(this, "Missing Participant ID",
                             "Please enter a participant ID before loading verification statistics.");
        participantIdCombo_->setFocus();
        return;
    }

    VerificationStats stats;

    if (!databaseManager_.loadVerificationStats(participantId, stats))
    {
        QMessageBox::warning(
            this,
            "Verification Statistics Failed",
            QString("Could not load verification statistics:\n%1")
                .arg(databaseManager_.lastErrorText()));
        return;
    }

    if (stats.totalCount == 0)
    {
        QMessageBox::information(
            this,
            "No Verification Data",
            QString("No verification results found for participant '%1'.")
                .arg(participantId));
        return;
    }

    const QString statsFilePath =
        VerificationStatsCsvWriter::defaultFilePath();

    if (!VerificationStatsCsvWriter::appendRow(statsFilePath, participantId, stats))
    {
        QMessageBox::warning(this, "Statistics Export Failed",
                             "Could not append verification statistics CSV file.");
        return;
    }

    QVector<VerificationResultExportRow> resultRows;

    if (!databaseManager_.loadVerificationResultRows(participantId, resultRows))
    {
        QMessageBox::warning(
            this,
            "Verification Export Failed",
            QString("Could not load verification result rows:\n%1")
                .arg(databaseManager_.lastErrorText()));
        return;
    }

    const QString resultsFilePath =
        VerificationResultsCsvWriter::defaultFilePath();

    if (!VerificationResultsCsvWriter::writeRows(resultsFilePath, resultRows))
    {
        QMessageBox::warning(this, "Verification Export Failed",
                             "Could not write verification results CSV file.");
        return;
    }

    QMessageBox::information(
        this,
        "Verification Statistics",
        QString(
            "Participant: %1\n\n"
            "Total verifications: %2\n"
            "Accepted: %3\n"
            "Rejected: %4\n\n"
            "Genuine attempts: %5\n"
            "Genuine accepted: %6\n"
            "Genuine rejected: %7\n"
            "FRR: %8%\n\n"
            "Impostor attempts: %9\n"
            "Impostor accepted: %10\n"
            "Impostor rejected: %11\n"
            "FAR: %12%\n\n"
            "Confusion matrix:\n"
            "True accept: %6\n"
            "False reject: %7\n"
            "False accept: %10\n"
            "True reject: %11\n\n"
            "Average score: %13\n"
            "Min score: %14\n"
            "Max score: %15"
            "\n\nExported to:\n%16\n\n"
            "Verification rows exported to:\n%17")
            .arg(participantId)
            .arg(stats.totalCount)
            .arg(stats.acceptedCount)
            .arg(stats.rejectedCount)
            .arg(stats.genuineCount)
            .arg(stats.genuineAcceptedCount)
            .arg(stats.genuineRejectedCount)
            .arg(stats.falseRejectRate * 100.0, 0, 'f', 2)
            .arg(stats.impostorCount)
            .arg(stats.impostorAcceptedCount)
            .arg(stats.impostorRejectedCount)
            .arg(stats.falseAcceptRate * 100.0, 0, 'f', 2)
            .arg(stats.averageTotalScore, 0, 'f', 3)
            .arg(stats.minTotalScore, 0, 'f', 3)
            .arg(stats.maxTotalScore, 0, 'f', 3)
            .arg(QDir::toNativeSeparators(statsFilePath))
            .arg(QDir::toNativeSeparators(resultsFilePath)));
}

void MainWindow::analyzeThresholds()
{
    const QString participantId = currentParticipantId();

    if (participantId.isEmpty())
    {
        QMessageBox::warning(this, "Missing Participant ID",
                             "Please enter a participant ID before running threshold analysis.");
        participantIdCombo_->setFocus();
        return;
    }

    QVector<VerificationResultExportRow> resultRows;

    if (!databaseManager_.loadVerificationResultRows(participantId, resultRows))
    {
        QMessageBox::warning(
            this,
            "Threshold Analysis Failed",
            QString("Could not load verification result rows:\n%1")
                .arg(databaseManager_.lastErrorText()));
        return;
    }

    if (resultRows.isEmpty())
    {
        QMessageBox::information(
            this,
            "No Verification Data",
            QString("No verification results found for participant '%1'.")
                .arg(participantId));
        return;
    }

    const ThresholdAnalysisResult analysis =
        ThresholdAnalysis::buildDefaultAnalysis(resultRows);

    if (analysis.genuineCount == 0 || analysis.impostorCount == 0)
    {
        QMessageBox::information(
            this,
            "Threshold Analysis Needs More Data",
            QString(
                "Threshold analysis needs both genuine and impostor attempts.\n\n"
                "Current data for participant '%1':\n"
                "Genuine attempts: %2\n"
                "Impostor attempts: %3")
                .arg(participantId)
                .arg(analysis.genuineCount)
                .arg(analysis.impostorCount));
        return;
    }

    const QString filePath =
        ThresholdAnalysisCsvWriter::defaultFilePath();

    if (!ThresholdAnalysisCsvWriter::writeRows(filePath, participantId, analysis.rows))
    {
        QMessageBox::warning(this, "Threshold Export Failed",
                             "Could not write threshold analysis CSV file.");
        return;
    }

    verificationThresholdSpinBox_->setValue(analysis.bestRow.threshold);

    QMessageBox::information(
        this,
        "Threshold Analysis",
        QString(
            "Participant: %1\n\n"
            "Recommended threshold: %2\n"
            "FAR: %3%\n"
            "FRR: %4%\n"
            "Balanced error: %5%\n"
            "FAR/FRR gap: %6%\n\n"
            "Samples used:\n"
            "Genuine attempts: %7\n"
            "Impostor attempts: %8\n\n"
            "The Threshold control was updated to the recommended value.\n\n"
            "Exported to:\n%9")
            .arg(participantId)
            .arg(analysis.bestRow.threshold, 0, 'f', 2)
            .arg(analysis.bestRow.falseAcceptRate * 100.0, 0, 'f', 2)
            .arg(analysis.bestRow.falseRejectRate * 100.0, 0, 'f', 2)
            .arg(analysis.bestRow.balancedErrorRate * 100.0, 0, 'f', 2)
            .arg(analysis.bestRow.absoluteRateGap * 100.0, 0, 'f', 2)
            .arg(analysis.genuineCount)
            .arg(analysis.impostorCount)
            .arg(QDir::toNativeSeparators(filePath)));
}
