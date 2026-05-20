#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#pragma once

#include "sessionfeaturevector.h"

#include <QSqlDatabase>
#include <QString>
#include <QVector>

struct TypingSession;
struct SessionSummary;

struct SavedSessionCheck
{
    int storedEventCount = 0;
    bool hasFeatureRow = false;
};

struct DatabaseStats
{
    int participantCount = 0;
    int sessionCount = 0;
    int eventCount = 0;
    int featureRowCount = 0;
};

struct VerificationResultRecord
{
    QString sessionId;
    QString participantId;
    QString attemptType;
    double totalScore = 0.0;
    double dwellDeviation = 0.0;
    double flightDeviation = 0.0;
    double threshold = 0.0;
    bool accepted = false;
    int trainingSessionCount = 0;
};

struct VerificationStats
{
    int totalCount = 0;
    int acceptedCount = 0;
    int rejectedCount = 0;

    double averageTotalScore = 0.0;
    double minTotalScore = 0.0;
    double maxTotalScore = 0.0;

    int genuineCount = 0;
    int genuineAcceptedCount = 0;
    int genuineRejectedCount = 0;

    int impostorCount = 0;
    int impostorAcceptedCount = 0;
    int impostorRejectedCount = 0;

    double falseAcceptRate = 0.0;
    double falseRejectRate = 0.0;

};

struct VerificationResultExportRow
{
    QString participantId;
    QString sessionId;
    QString attemptType;
    QString createdAtUtc;
    bool accepted = false;
    double threshold = 0.0;
    double totalScore = 0.0;
    double dwellDeviation = 0.0;
    double flightDeviation = 0.0;
    int trainingSessionCount = 0;
};


class DatabaseManager
{
public:

    bool saveSession(const TypingSession &session,
                     const SessionSummary &summary,
                     const SessionFeatureVector &features);

    bool loadSavedSessionCheck(const QString &sessionId,
                               SavedSessionCheck &check);

    bool loadDatabaseStats(DatabaseStats &stats);

    bool loadTrainingFeatureVectors(const QString &participantId,
                                    QVector<SessionFeatureVector> &features);

    bool saveVerificationResult(const VerificationResultRecord &result);

    bool loadVerificationStats(const QString &participantId,
                               VerificationStats &stats);

    bool loadVerificationResultRows(const QString &participantId,
                                    QVector<VerificationResultExportRow> &rows);

    DatabaseManager();
    ~DatabaseManager();

    bool open();

    QString databaseFilePath() const;
    QString lastErrorText() const;

private:
    bool open(const QString &databasePath);
    bool initializeSchema();
    bool ensureParticipantExists(const QString &participantId);
    void close();

    QSqlDatabase database_;
    QString lastErrorText_;
};


#endif // DATABASEMANAGER_H
