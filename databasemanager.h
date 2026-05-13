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
